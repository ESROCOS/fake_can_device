/*
    ESROCOS fake CAN device
    @author Moritz Schilling
    @date 2018-07-20
*/


#include <linux/can.h>
#include <linux/can/raw.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>

#include <pthread.h>

#include <iostream>
#include <string>
#include <cstring>

/* Actuator thread */
pthread_t threadId;
static float velocity_ref_deg = 0.f;
static float position_deg = 0.f;
static struct timespec lastTime;
static void *updateActuator(void *)
{
    while (true)
    {
        /*First calculate the delta t*/
        struct timespec currentTime;
        clock_gettime(CLOCK_MONOTONIC, &currentTime);
        float delta_t = (currentTime.tv_sec + currentTime.tv_nsec / 1000000000.f) - (lastTime.tv_sec + lastTime.tv_nsec / 1000000000.f);
        lastTime = currentTime;
        /*Then update position*/
        position_deg += velocity_ref_deg * delta_t;
    }
}

int main(int argc, char** argv)
{
    const char* interface;
    struct sockaddr_can addr;
    struct ifreq ifr;
    int sockfd;

    if (argc < 2)
    {
        std::cerr << "Please provide an interface name\n";
        return 1;
    }
    interface = argv[1];

    sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sockfd < 0) {
        std::cerr << "Could not open socket\n";
        return 2;
    }
    std::strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1)
    {
        std::cerr << "Could not get index for " << std::string(interface) << "\n";
        return 3;
    }
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        std::cerr << "Could bind to socket\n";
        return 4;
    }

    /*Start actuator thread*/
    pthread_create(&threadId, NULL, updateActuator, NULL);

    while (true)
    {
        struct can_frame request;
        int numBytes = read(sockfd, &request, sizeof(request));
        if (numBytes < sizeof(request))
            continue;
        if ((request.can_id & CAN_SFF_MASK) == 0x7c0)
        {
            std::cout << "[fake-can-dev] Got telemetry request. Sending response\n";
            uint16_t position = position_deg * 0xFFFF / 360.f;
            // Send response
            struct can_frame response;
            response.can_id = 0x1a0;
            response.can_dlc = 6;
            response.data[2] = position & 0xFF;
            response.data[3] = (position >> 8) & 0xFF;
            write(sockfd, &response, sizeof(response));
        }
        if ((request.can_id & CAN_SFF_MASK) == 0x182)
        {
            if (request.can_dlc != 3)
            {
                std::cout << "[fake-can-dev] Telecommand has wrong length. Got " << (unsigned int)(request.can_dlc) << "\n";
                continue;
            }
            if (request.data[0] == 0)
            {
                // Idle mode. Set velocity_ref to 0
                std::cout << "[fake-can-dev] Idle mode. Setting velocity to 0\n";
                velocity_ref_deg = 0.f;
                continue;
            }
            if (request.data[0] == 2)
            {
                int16_t velocity_ref;
                velocity_ref = request.data[2];
                velocity_ref <<= 8;
                velocity_ref += request.data[1];
                std::cout << "[fake-can-dev] Velocity mode. Setting velocity to " << velocity_ref << "\n";
                /* Maximum velocity is 1rpm ... that means 360deg/60s = 6deg/s */
                velocity_ref_deg = velocity_ref * 6.f / 0xFFFF;
                continue;
            }
            std::cout << "[fake-can-dev] Got unknown telecommand\n";
        }
    }

    return 0;
}
