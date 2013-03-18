/*
 * Server.cpp
 *
 *  Created on: 15 Mar 2013
 *      Author: hassan
 */

#include "../header/Server.h"

void Server::initialise()
{
    cout << "Initialising server. Please start the client" << endl;
    struct sockaddr_in server;

    /* open socket */
    if ((serversock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        cerr << "socket() failed";
        exit(1);
    }

    /* setup server's IP and port */
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_send);
    server.sin_addr.s_addr = INADDR_ANY;

    /* bind the socket */
    if (bind(serversock, (const sockaddr*) &server, sizeof(server)) == -1)
    {
        //quit("bind() failed", 1);
        cerr << "bind() failed";
        exit(1);
    }

    /* wait for connection */
    if (listen(serversock, 2) == -1)
    {
    }

    /* accept a client */
    if ((clientsock = accept(serversock, NULL, NULL)) == -1)
    {
        cerr << "accept() failed";
        exit(1);
    }

    hasClient = 1;
}

void Server::sendData(StereoPair &input, Mat &image, bool shouldBrake, Car& car, string message)
{
    try {
        sendBrakeData(shouldBrake);
        displayImage(input, image);
        sendMessage(message);
        sendSpeed(car.getSpeedString());
        checkForData();
    } catch (Exception e)
    {
        car.brake();
        cout << e.msg << endl;
    }

}

void Server::displayImage(StereoPair &input, Mat &image)
{
    if (hasClient)
    {
        switch (streamDisplayType)
        {
            case FLAGS::LEFT:
                sendImageToClient(input.leftImage);
                break;
            case FLAGS::RIGHT:
                sendImageToClient(input.rightImage);
                break;
            case FLAGS::DISPARITY:
                sendImageToClient(image);
                break;
            default:
                break;
        }
    }
    else
    {
        listenForClient();
    }
}

void Server::sendImageToClient(Mat &image)
{
    IplImage *img1 = new IplImage(image);

    int bytes = send(clientsock, img1->imageData, img1->imageSize, 0);

    /* if something went wrong, restart the connection */
    if (bytes != img1->imageSize)
    {
        throw "sendImageToClient failed";
//        fprintf(stderr, "Connection closed 2.\n");
//        close(clientsock);
//        hasClient = 0;
    }

}

void  Server::sendBrakeData(bool shouldBrake)
{
    uint8_t brake[1];

    if (shouldBrake)
        brake[0] = 0x01;
    else
        brake[0] = 0x00;

    int bytes = send(clientsock, brake, sizeof(brake), 0);

    if (bytes != sizeof(brake))
    {
        throw "sendBrakeData failed";
    }
}

void Server::sendMessage(string message)
{
    int bytes = send(clientsock, message.data(), 32, 0);

    if (bytes != 32)
        {
            throw "sendMessage failed";
        }
}

void Server::sendSpeed(string speed)
{
    int bytes = send(clientsock, speed.data(), 5, 0);

    if (bytes != 5)
    {
        throw "sendSpeed failed";
    }
}

void Server::checkForData()
{
    int n = 0;
    fd_set input;
    struct timeval timeout;

    FD_ZERO(&input);
    FD_SET(clientsock, &input);

    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

    n = select(clientsock + 1, &input, NULL, NULL, &timeout);
    /* start sending images */
    if (n < 0)
    {
        perror("select failed");
    }
    else if (n == 0)
    {

    }
    else if (FD_ISSET(clientsock,&input))
    {
        uint8_t buffer[1];
        recv(clientsock, buffer, sizeof(buffer), 0);

        switch (buffer[0])
        {
            case 0x00:
                streamDisplayType = FLAGS::LEFT;
                break;
            case 0x01:
                streamDisplayType = FLAGS::RIGHT;
                break;
            case 0x02:
                streamDisplayType = FLAGS::DISPARITY;
                break;
            default:
                streamDisplayType = FLAGS::LEFT;
                break;
        }

    }
}

void Server::listenForClient()
{
    /* wait for connection */
    if (listen(serversock, 2) == -1)
    {
    }

    /* accept a client */
    if ((clientsock = accept(serversock, NULL, NULL)) == -1)
    {
        cerr << "accept() failed";
        exit(1);
    }
    hasClient = 1;
}

