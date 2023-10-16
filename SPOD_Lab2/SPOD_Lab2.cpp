#include <iostream>
#include "mpi.h"

#define WORK_REQUEST 0x001
#define WORK_RESPONSE 0x010
#define WORK_END 0x100
#define INFO_SIZE 35
#define MIN_PROCESSES_COUNT 3

typedef struct package_s {
    int destination;
    char information[INFO_SIZE];
} package;

int main(int argc, char* argv[])
{
    int processesCount, processRank, receivedRank;
    double start, end;
    MPI_Status status;
    package message;
    bool working = true;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &processesCount);

    if (processesCount < MIN_PROCESSES_COUNT) {
        puts("\t\t\t\t**************************************\n");
        puts("\t\t\t\t**Requires at least three processes!**\n");
        puts("\t\t\t\t**************************************\n");
        exit(-1);
    }

    const int nItems = 2;
    int blockLengths[2] = { 1, INFO_SIZE };
    MPI_Datatype types[2] = { MPI_INT, MPI_CHAR };
    MPI_Datatype MPI_PACKAGE;
    MPI_Aint offsets[2];

    offsets[0] = offsetof(package, destination);
    offsets[1] = offsetof(package, information);

    MPI_Type_create_struct(nItems, blockLengths, offsets, types, &MPI_PACKAGE);
    MPI_Type_commit(&MPI_PACKAGE);

    MPI_Comm_rank(MPI_COMM_WORLD, &processRank);

    if (processRank == 0) {
        for (int i = 1; i < processesCount; ++i) {
            int correctMPI;

            correctMPI = MPI_Recv(&message, 1, MPI_PACKAGE, MPI_ANY_SOURCE, WORK_REQUEST, MPI_COMM_WORLD, &status);

            int messageSource = status.MPI_SOURCE;
            int messageDestination = message.destination;
            char* messageInformation = message.information;

            correctMPI = MPI_Send(&message, 1, MPI_PACKAGE, messageDestination, WORK_REQUEST, MPI_COMM_WORLD);
            correctMPI = MPI_Recv(&message, 1, MPI_PACKAGE, messageDestination, WORK_RESPONSE, MPI_COMM_WORLD, &status);
            correctMPI = MPI_Send(&message, 1, MPI_PACKAGE, messageSource, WORK_RESPONSE, MPI_COMM_WORLD);

            if (correctMPI == MPI_SUCCESS) {
                printf("Process %d send package to process %d\n", messageSource, messageDestination);
                printf("Process %d got package from process %d. Information: %s\n", messageDestination, messageSource, messageInformation);
                printf("Process %d got confirmation from process %d\n", messageSource, messageDestination);
            }
            else {
                printf("Error while using MPI functions!\n");
                exit(-2);
            }
        }
        for (int i = 1; i < processesCount; ++i) {
            MPI_Send(&message, 1, MPI_PACKAGE, i, WORK_END, MPI_COMM_WORLD);
        }
    }
    else {
        srand(time(NULL) + processRank * processesCount);
        int destination;

        do {
            destination = rand() % (processesCount - 1) + 1;
        } while (destination == processRank);

        char information[INFO_SIZE];
        sprintf_s(information, "This message from process %d", processRank);

        message.destination = destination;
        strcpy_s(message.information, information);

        MPI_Send(&message, 1, MPI_PACKAGE, 0, WORK_REQUEST, MPI_COMM_WORLD);

        while (working) {
            MPI_Recv(&message, 1, MPI_PACKAGE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == WORK_REQUEST) {
                MPI_Send(&message, 1, MPI_PACKAGE, 0, WORK_RESPONSE, MPI_COMM_WORLD);
            }
            else if (status.MPI_TAG == WORK_END) {
                working = false;
            }
        }
    }

    MPI_Finalize();
}
