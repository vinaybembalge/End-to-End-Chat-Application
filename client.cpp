#include <iostream>
#include <cstdlib>
#include <WinSock2.h>
#include <WS2tcpip.h> // Required for inet_pton
#include <thread>
#include <string>
#include <atomic> // For atomic flag

using namespace std;

#pragma comment(lib, "ws2_32.lib") // Winsock Library

bool Initialize()
{
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

// Atomic flag to handle graceful exit across threads
atomic<bool> shouldExit(false);

void SendMsg(SOCKET s)
{
    cout << "Enter your chat name: ";
    string name;
    getline(cin, name);
    string message;

    while (!shouldExit)
    {
        getline(cin, message);
        if (message == "exit")
        {
            shouldExit = true; // Notify the receiver thread to stop
            cout << "Stopping the chat..." << endl;
            break;
        }

        string msg = name + ": " + message;
        int bytesent = send(s, msg.c_str(), msg.length(), 0); // Send message to server
        if (bytesent == SOCKET_ERROR)
        {
            cout << "Failed to send data to server." << endl;
            break;
        }
    }
}

void ReceiveMsg(SOCKET s)
{
    char buffer[4096];
    int recvlength;

    while (!shouldExit)
    {
        recvlength = recv(s, buffer, sizeof(buffer) - 1, 0); // Receive message from server
        if (recvlength > 0)
        {
            buffer[recvlength] = '\0'; // Null-terminate the received data
            cout << buffer << endl; // Display message
        }
        else if (recvlength == 0)
        {
            cout << "Connection closed by server." << endl;
            shouldExit = true; // Notify sender thread to stop
            break;
        }
        else
        {
            cout << "Failed to receive data from server." << endl;
            shouldExit = true; // Notify sender thread to stop
            break;
        }
    }
}

int main()
{
    if (!Initialize())
    {
        cerr << "Failed to initialize." << endl;
        return 1;
    }
    cout << "Client 1" << endl;

    // Create a socket
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
    {
        cerr << "Failed to create socket." << endl;
        WSACleanup();
        return 1;
    }

    int port = 12345;
    string serveraddress = "127.0.0.1"; // localhost

    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port); // Convert to network byte order

    // Corrected inet_pton usage
    if (inet_pton(AF_INET, serveraddress.c_str(), &(serveraddr.sin_addr)) <= 0)
    {
        cerr << "Invalid address or address not supported." << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    // Connect to server
    if (connect(s, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR)
    {
        cerr << "Failed to connect to server." << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }
    cout << "Connected to server successfully." << endl;

    // Launch threads for sending and receiving messages
    thread senderThread(SendMsg, s);
    thread receiverThread(ReceiveMsg, s);

    senderThread.join(); // Wait for sender thread to finish
    receiverThread.join(); // Wait for receiver thread to finish

    // Clean up
    closesocket(s);
    WSACleanup();

    return 0;
}
