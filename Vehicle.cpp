#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <openssl/err.h>
#include <oqs/sig_falcon.h>
#include "Vehicle.h"
#include <openssl/pem.h>
#include <oqs/oqs.h>
#include <thread>

std::string Vehicle::get_hostname() {
   return hostname;
}

struct test {
    std::string word = "hello";
};

void print_run_config(std::string mode, std::string signature_alg, std::string cert_alg) {
    std::cout << "Running " << mode << " with " << signature_alg << " signatures and " << cert_alg << " certificates" << std::endl;
}

void Vehicle::transmit(int num_msgs,ArgumentParser arg_pars) {

    // create socket and send data

    int sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    if(arg_pars.istest)
        servaddr.sin_port = htons(6666);
    else
        servaddr.sin_port = htons(52001);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    int n, len;

    if(arg_pars.sig_alg == ECDSA && arg_pars.cert_alg == ECDSA)
        print_run_config("transmitter", "ECDSA", "ECDSA");
    else if(arg_pars.sig_alg == FALCON && arg_pars.cert_alg == FALCON)
        print_run_config("transmitter", "FALCON", "FALCON");

    for(int i =0; i < num_msgs; i++) {

        if(arg_pars.sig_alg == ECDSA && arg_pars.cert_alg == ECDSA) {

            ecdsa_spdu next_spdu;
            generate_ecdsa_spdu(next_spdu);
            sendto(sockfd, (struct ecdsa_spdu *)&next_spdu, sizeof(next_spdu), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        }
        else if(arg_pars.sig_alg == FALCON && arg_pars.cert_alg == FALCON) {

            falcon_spdu next_spdu;
            generate_falcon_spdu(next_spdu);
            sendto(sockfd, (struct ecdsa_spdu *)&next_spdu, sizeof(next_spdu), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }

    close(sockfd);

}

void Vehicle::receive(int num_msgs, ArgumentParser arg_pars) {

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    std::cout << arg_pars.istest << std::endl;
    uint16_t port = arg_pars.istest ? 6666 : 4444;
    std::cout << port << std::endl;
    servaddr.sin_port = htons(port);

    if(bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Socket bind failed");
        exit(EXIT_FAILURE);
    }

    int len, n;

    len = sizeof(cliaddr);

    if(arg_pars.sig_alg == ECDSA && arg_pars.cert_alg == ECDSA) {
        print_run_config("receiver", "ECDSA", "ECDSA");
        ecdsa_spdu incoming_spdu;

        // this is to prevent a truly infinite loop
        int received_message_counter = 0;
        while (received_message_counter < num_msgs) {
            recvfrom(sockfd, (struct ecdsa_spdu *) &incoming_spdu, sizeof(ecdsa_spdu), 0, (struct sockaddr *) &cliaddr,
                     (socklen_t *) len);

            print_spdu(incoming_spdu);
            verify_message_ecdsa(incoming_spdu);
            received_message_counter++;
            std::cout<< received_message_counter << std::endl;
        }
        get_average_verify_times();
        exit(0);

    } else if(arg_pars.sig_alg == FALCON && arg_pars.cert_alg == FALCON) {
        print_run_config("receiver", "FALCON", "FALCON");
        falcon_spdu incoming_spdu;

        // this is to prevent a truly infinite loop
        int received_message_counter = 0;
        while (received_message_counter < num_msgs) {
            recvfrom(sockfd, (struct falcon_spdu *) &incoming_spdu, sizeof(falcon_spdu), 0, (struct sockaddr *) &cliaddr,
                     (socklen_t *) len);
            verify_message_falcon(incoming_spdu);
            received_message_counter++;
        }
        get_average_verify_times();
        exit(0);
    }


}

void Vehicle::generate_ecdsa_spdu(Vehicle::ecdsa_spdu &spdu) {

    // BSM
    spdu.data.signedData.tbsData.message = generate_bsm();

    // timestamp
    using timestamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;
    timestamp ts = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    spdu.data.signedData.tbsData.headerInfo.timestamp = ts;

    size_t size = sizeof(spdu.data.signedData.tbsData);

    spdu.data.signedData.cert = vehicle_certificate_ecdsa;

    // sign the certificate
    auto start = std::chrono::high_resolution_clock::now();

    unsigned char certificate_digest[SHA256_DIGEST_LENGTH];
    sha256sum(&spdu.data.signedData.cert,sizeof(spdu.data.signedData.cert), certificate_digest);
    ecdsa_sign(certificate_digest, cert_private_ec_key, &certificate_buffer_length, certificate_signature);

    auto stop = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    // std::cout << "Sign time: " << duration.count() << std::endl;

    sign_times.push_back(duration.count());

    // copy the certificate signature buffer and certificate signature into the SPDU
    spdu.certificate_signature_buffer_length = certificate_buffer_length;
    for(int i = 0; i < certificate_buffer_length; i++) {
        spdu.data.certificate_signature[i] = certificate_signature[i];
    }

    sign_message_ecdsa(spdu);

}

void Vehicle::generate_falcon_spdu(Vehicle::falcon_spdu &spdu) {

    spdu.data.signedData.tbsData.message = generate_bsm();
    // timestamp
    using timestamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;
    timestamp ts = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    spdu.data.signedData.tbsData.headerInfo.timestamp = ts;

    size_t size = sizeof(spdu.data.signedData.tbsData);

    spdu.data.signedData.cert = vehicle_certificate_falcon;

    auto start_time = std::chrono::high_resolution_clock::now();

    // sign the certificate
    unsigned char signature[OQS_SIG_falcon_512_length_signature];
    size_t signature_length;
    auto cert_bytes = reinterpret_cast<uint8_t*>(&spdu.data.signedData.cert);
    size_t cert_length = sizeof(spdu.data.signedData.cert);

    falcon_sign(signature, signature_length, cert_bytes, cert_length, falcon_private_key);

    auto stop_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop_time - start_time);
    sign_times.push_back(duration.count());

    for(int i = 0; i < OQS_SIG_falcon_512_length_signature; i++)
        spdu.data.certificate_signature[i] = signature[i];

    sign_message_falcon(spdu);

}

bsm Vehicle::generate_bsm() {
    bsm new_bsm = {10, 10, 10, 30, 0};
    return new_bsm;
}

void Vehicle::print_bsm(Vehicle::ecdsa_spdu &spdu) {
    for(int i = 0; i < 80; i++)
        std::cout << "-";
    std::cout << std::endl;
    std::cout << "BSM received!" << std::endl;
    std::cout << "Location:\t";
    std::cout << spdu.data.signedData.tbsData.message.latitude;
    std::cout << ", ";
    std::cout << spdu.data.signedData.tbsData.message.longitude;
    std::cout <<", ";
    std::cout << spdu.data.signedData.tbsData.message.elevation;
    std::cout << std::endl;
    std::cout << "Speed:\t\t" << spdu.data.signedData.tbsData.message.speed << std::endl;
    std::cout << "Heading:\t" << spdu.data.signedData.tbsData.message.heading << std::endl;
}

void Vehicle::print_spdu(Vehicle::ecdsa_spdu &spdu) {
    std::cout << "Timestamp:\t" << std::chrono::system_clock::to_time_t(spdu.data.signedData.tbsData.headerInfo.timestamp) << std::endl;
}

void Vehicle::sign_message_ecdsa(Vehicle::ecdsa_spdu &spdu) {

    unsigned char hash[SHA256_DIGEST_LENGTH];
    sha256sum(&spdu.data.signedData.tbsData, sizeof(spdu.data.signedData.tbsData), hash);

    unsigned int signature_buffer_length = ECDSA_size(private_ec_key);
    auto signature = (unsigned char*) OPENSSL_malloc(signature_buffer_length);
    ecdsa_sign(hash, private_ec_key, &signature_buffer_length, signature);

    spdu.signature_buffer_length = signature_buffer_length;

    for(int i = 0; i < signature_buffer_length; i++) {
        spdu.signature[i] = signature[i];
    }

}

void Vehicle::verify_message_ecdsa(Vehicle::ecdsa_spdu &spdu) {

    auto start = std::chrono::high_resolution_clock::now();

    unsigned char certificate_hash[SHA256_DIGEST_LENGTH];
    sha256sum(&spdu.data.signedData.cert, sizeof(spdu.data.signedData.cert), certificate_hash);

    int cert_result = ecdsa_verify(certificate_hash, spdu.data.certificate_signature, &spdu.certificate_signature_buffer_length, cert_private_ec_key);
    switch(cert_result) {
        case -1:
            perror("Error in call to ECDSA_verify");
            exit(EXIT_FAILURE);
        case 0:
            std::cout << "INVALID certificate signature" << std::endl;
            break;
        case 1:
            std::cout << "Valid certificate signature" << std::endl;
            break;
        default:
            break;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    sha256sum(&spdu.data.signedData.tbsData, sizeof(spdu.data.signedData.tbsData), hash);

    int result = ecdsa_verify(hash, spdu.signature, &spdu.signature_buffer_length, private_ec_key);

    switch(result) {
        case -1:
            perror("Error in call to ECDSA_verify");
            exit(EXIT_FAILURE);
        case 0:
            std::cout << "INVALID message signature" << std::endl;
            break;
        case 1:
            std::cout << "Valid message signature" << std::endl;
            break;
        default:
            break;
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    verify_times.push_back(duration.count());
}

void Vehicle::sign_message_falcon(Vehicle::falcon_spdu &spdu) {

    unsigned char signature[OQS_SIG_falcon_512_length_signature];
    size_t signature_length;

    auto spdu_bytes = reinterpret_cast<uint8_t*>(&spdu.data.signedData.tbsData);
    size_t message_length = sizeof(spdu.data.signedData.tbsData);

    falcon_sign(signature, signature_length, spdu_bytes, message_length, falcon_private_key);

    spdu.signature_buffer_length = signature_length;

    for(int i = 0; i < signature_length; i++)
        spdu.signature[i] = signature[i];

    for(int i = 0; i < OQS_SIG_falcon_512_length_public_key; i++)
        spdu.data.signedData.cert.signing_public_key[i] = falcon_public_key[i];

}

void Vehicle::verify_message_falcon(Vehicle::falcon_spdu &spdu) {

    auto start = std::chrono::high_resolution_clock::now();

    auto spdu_bytes = reinterpret_cast<uint8_t*>(&spdu.data.signedData.tbsData);
    size_t message_length = sizeof(spdu.data.signedData.tbsData);

    uint8_t public_key_from_spdu[OQS_SIG_falcon_512_length_public_key];
    for(int i = 0; i < OQS_SIG_falcon_512_length_public_key; i++)
        public_key_from_spdu[i] = spdu.data.signedData.cert.signing_public_key[i];

    bool result = falcon_verify(spdu_bytes, message_length, spdu.signature, spdu.signature_buffer_length, public_key_from_spdu);

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    verify_times.push_back(duration.count());

    result ? std::cout << "Valid message signature" << std::endl : std::cout << "INVALID message signature" << std::endl;
}

void Vehicle::load_key_from_file_ecdsa(const char* filepath, EC_KEY *&key_to_store){
    FILE *fp = fopen(filepath,"r");
    if(fp != nullptr) {
        EVP_PKEY *key = nullptr;
        PEM_read_PrivateKey(fp, &key, nullptr, nullptr);
        key_to_store = nullptr;
        if (!key) {
            perror("Error while loading the key from file\n");
            exit(EXIT_FAILURE);
        }
        if (!(key_to_store = EVP_PKEY_get1_EC_KEY(key))) {
            perror("Error while getting EC key from loaded key\n");
            exit(EXIT_FAILURE);
        }
    }
    else {
        std::cout << "Error while opening file from path. Error number : " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

}

void Vehicle::get_average_sign_times() {

    long int total = 0;

    for(int64_t i : sign_times) {
        total += i;
    }

    long double average = total / sign_times.size();

    std::cout << "Average sign time for " << sign_times.size();
    std::cout << " runs was " << average << " microseconds" << std::endl;

}

void Vehicle::get_average_verify_times() {

    long int total = 0;

    for(int64_t i : verify_times) {
        total += i;
    }

    long double average = total / verify_times.size();

    std::cout << "Average verify time for " << verify_times.size();
    std::cout << " runs was " << average << " microseconds" << std::endl;
}