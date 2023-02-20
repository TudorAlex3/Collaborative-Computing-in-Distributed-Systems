#include "mpi.h"
#include <iostream>
#include <fstream>
#include <stddef.h>
#include <vector>
#include <math.h>

using namespace std;

typedef struct
{
    int rank0_workers[100];
    int rank1_workers[100];
    int rank2_workers[100];
    int rank3_workers[100];
} topology;

void show_topology(topology full_topology, int rank);
void message_log(int source, int dest);

int main(int argc, char *argv[])
{
    int numtasks, rank;
    int workers_number;
    topology full_topology;

    MPI_Datatype top, oldtypes[4];
    int blockcounts[4];
    MPI_Aint offsets[4];
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // campul rank0_workers
    offsets[0] = offsetof(topology, rank0_workers);
    oldtypes[0] = MPI_INT;
    blockcounts[0] = numtasks;

    // campul rank1_workers
    offsets[1] = offsetof(topology, rank1_workers);
    oldtypes[1] = MPI_INT;
    blockcounts[1] = numtasks;

    // campul rank2_workers
    offsets[2] = offsetof(topology, rank2_workers);
    oldtypes[2] = MPI_INT;
    blockcounts[2] = numtasks;

    // campul rank3_workers
    offsets[3] = offsetof(topology, rank3_workers);
    oldtypes[3] = MPI_INT;
    blockcounts[3] = numtasks;

    MPI_Type_create_struct(4, blockcounts, offsets, oldtypes, &top);
    MPI_Type_commit(&top);

    // Variabile utile pentru impartirea vectorului
    // plus - worker-ul ia un element in plus pentru ca impartirea
    // sa fie echitabila
    int elements = atoi(argv[1]);
    int index;
    int elements_worker;
    int plus;

    // Procesul 0
    if (rank == 0)
    {
        // Citesc din fisier si adaug procesele din acelasi cluster
        ifstream cluster_file("cluster0.txt");
        string line;

        cluster_file >> workers_number;
        full_topology.rank0_workers[0] = workers_number;

        int x;
        int i = 1;
        while (cluster_file >> x)
        {
            full_topology.rank0_workers[i] = x;
            i++;
        }
        cluster_file.close();

        // Trimit topologia incompleta la cluster 3
        MPI_Send(&full_topology, 1, top, 3, 0, MPI_COMM_WORLD);
        message_log(rank, 3);

        // Primesc topologia completa de la cluster 3
        MPI_Recv(&full_topology, 1, top, 3, 0, MPI_COMM_WORLD, &status);
        show_topology(full_topology, rank);

        // Trimit topologia completa la workers
        for (i = 1; i <= workers_number; i++)
        {
            MPI_Send(&full_topology, 1, top,
                     full_topology.rank0_workers[i], 0, MPI_COMM_WORLD);
            message_log(rank, full_topology.rank0_workers[i]);
        }

        // Numarul total de workers si generarea vectorului
        int number_of_workers = 0;
        number_of_workers += full_topology.rank0_workers[0];
        number_of_workers += full_topology.rank1_workers[0];
        number_of_workers += full_topology.rank2_workers[0];
        number_of_workers += full_topology.rank3_workers[0];

        int vector[elements];
        for (int k = 0; k <= elements - 1; k++)
        {
            vector[k] = elements - k - 1;
        }

        // Aflu cate elemente revin fiecarui worker
        elements_worker = floor(elements / number_of_workers);
        plus = elements % number_of_workers;

        // Trimit elementele corespunzatoare fiecarui worker
        // Memorez pentru fiecare indexul de start si numarul de elemente
        int index_workers[workers_number + 1];
        int elements_workers[workers_number + 1];

        index = 0;
        for (i = 1; i <= workers_number; i++)
        {
            int elements_this_worker = elements_worker;
            if (plus > 0)
            {
                elements_this_worker++;
                plus--;
            }

            index_workers[i] = index;
            elements_workers[i] = elements_this_worker;

            MPI_Send(&elements_this_worker, 1, MPI_INT,
                     full_topology.rank0_workers[i], 0, MPI_COMM_WORLD);
            MPI_Send(vector + index, elements_this_worker, MPI_INT,
                     full_topology.rank0_workers[i], 0, MPI_COMM_WORLD);
            message_log(rank, full_topology.rank0_workers[i]);

            index += elements_this_worker;
        }

        // Trimit vectorul la cluster 3
        // Folosesc index pentru a specifica de la ce index incepe zona
        // atribuita pentru workerii clusterului 3
        MPI_Send(vector, elements, MPI_INT, 3, 0, MPI_COMM_WORLD);
        MPI_Send(&elements_worker, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
        MPI_Send(&index, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
        MPI_Send(&plus, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
        message_log(rank, 3);

        // Primesc vectorul complet de la cluster 3
        MPI_Recv(vector, elements, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);

        // Primesc mesajele de la workerii personali si updatez rezultatul
        // final cu valorile primite
        int current_index;
        for (i = 1; i <= workers_number; i++)
        {
            int partial_vector[elements_workers[i]];
            current_index = index_workers[i];

            MPI_Recv(partial_vector, elements_workers[i], MPI_INT,
                     full_topology.rank0_workers[i], 0, MPI_COMM_WORLD, &status);

            for (int j = current_index, k = 0; k < elements_workers[i]; j++, k++)
            {
                vector[j] = partial_vector[k];
            }
        }

        cout << "Rezultat: ";
        for (i = 0; i < elements; i++)
        {
            if (i + 1 < elements)
                cout << vector[i] << " ";
            else
                cout << vector[i] << endl;
        }
    }
    else if (rank == 1)
    {
        ifstream cluster_file("cluster1.txt");
        string line;

        // Primesc topologia icompletata de la cluster 2
        MPI_Recv(&full_topology, 1, top, 2, 0, MPI_COMM_WORLD, &status);

        // Completez topologia cu workers pentru cluster 1
        cluster_file >> workers_number;
        full_topology.rank1_workers[0] = workers_number;

        int x;
        int i = 1;
        while (cluster_file >> x)
        {
            full_topology.rank1_workers[i] = x;
            i++;
        }
        cluster_file.close();

        // Am completat topologia, o trimit inapoi
        show_topology(full_topology, rank);
        MPI_Send(&full_topology, 1, top, 2, 0, MPI_COMM_WORLD);
        message_log(rank, 2);

        // Trimit topologia completa la workers
        for (i = 1; i <= workers_number; i++)
        {
            MPI_Send(&full_topology, 1, top,
                     full_topology.rank1_workers[i], 0, MPI_COMM_WORLD);
            message_log(rank, full_topology.rank1_workers[i]);
        }

        // Primesc vector incomplet de la cluster 2
        int vector[elements];
        MPI_Recv(vector, elements, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&elements_worker, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&index, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&plus, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);

        // Trimit elementele corespunzatoare fiecarui worker
        // Memorez pentru fiecare indexul de start si numarul de elemente
        int index_workers[workers_number + 1];
        int elements_workers[workers_number + 1];

        for (i = 1; i <= workers_number; i++)
        {
            int elements_this_worker = elements_worker;
            if (plus > 0)
            {
                elements_this_worker++;
                plus--;
            }

            index_workers[i] = index;
            elements_workers[i] = elements_this_worker;

            MPI_Send(&elements_this_worker, 1, MPI_INT,
                     full_topology.rank1_workers[i], 0, MPI_COMM_WORLD);
            MPI_Send(vector + index, elements_this_worker, MPI_INT,
                     full_topology.rank1_workers[i], 0, MPI_COMM_WORLD);
            message_log(rank, full_topology.rank1_workers[i]);

            index += elements_this_worker;
        }

        // Primesc mesajele de la workerii personali si updatez rezultatul
        // final cu valorile primite
        int current_index;
        for (i = 1; i <= workers_number; i++)
        {
            int partial_vector[elements_workers[i]];
            current_index = index_workers[i];

            MPI_Recv(partial_vector, elements_workers[i], MPI_INT,
                     full_topology.rank1_workers[i], 0, MPI_COMM_WORLD, &status);

            for (int j = current_index, k = 0; k < elements_workers[i]; j++, k++)
            {
                vector[j] = partial_vector[k];
            }
        }

        // Trimit vectorul final la cluster 2
        MPI_Send(vector, elements, MPI_INT, 2, 0, MPI_COMM_WORLD);
        message_log(rank, 2);
    }
    else if (rank == 2)
    {
        ifstream cluster_file("cluster2.txt");
        string line;

        // Primesc topologia incompletata de la cluster 3
        MPI_Recv(&full_topology, 1, top, 3, 0, MPI_COMM_WORLD, &status);

        // Completez topologia cu workers pentru cluster 2
        cluster_file >> workers_number;
        full_topology.rank2_workers[0] = workers_number;
        int x;
        int i = 1;
        while (cluster_file >> x)
        {
            full_topology.rank2_workers[i] = x;
            i++;
        }
        cluster_file.close();

        // Trimit topologia incompleta la cluster 1
        MPI_Send(&full_topology, 1, top, 1, 0, MPI_COMM_WORLD);
        message_log(rank, 1);

        // Primesc topologia completa de la cluster 1 si o trimit la 3
        MPI_Recv(&full_topology, 1, top, 1, 0, MPI_COMM_WORLD, &status);
        show_topology(full_topology, rank);
        MPI_Send(&full_topology, 1, top, 3, 0, MPI_COMM_WORLD);
        message_log(rank, 3);

        for (i = 1; i <= workers_number; i++)
        {
            MPI_Send(&full_topology, 1, top,
                     full_topology.rank2_workers[i], 0, MPI_COMM_WORLD);
            message_log(rank, full_topology.rank2_workers[i]);
        }

        // Primesc vector incomplet de la cluster 3
        int vector[elements];
        MPI_Recv(vector, elements, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&elements_worker, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&index, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&plus, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);

        // Trimit elementele corespunzatoare fiecarui worker
        // Memorez pentru fiecare indexul de start si numarul de elemente
        int index_workers[workers_number + 1];
        int elements_workers[workers_number + 1];

        for (i = 1; i <= workers_number; i++)
        {
            int elements_this_worker = elements_worker;
            if (plus > 0)
            {
                elements_this_worker++;
                plus--;
            }

            index_workers[i] = index;
            elements_workers[i] = elements_this_worker;

            MPI_Send(&elements_this_worker, 1, MPI_INT,
                     full_topology.rank2_workers[i], 0, MPI_COMM_WORLD);
            MPI_Send(vector + index, elements_this_worker, MPI_INT,
                     full_topology.rank2_workers[i], 0, MPI_COMM_WORLD);
            message_log(rank, full_topology.rank2_workers[i]);

            index += elements_this_worker;
        }

        // Trimit vectorul la cluster 1
        // Folosesc index pentru a specifica de la ce index incepe zona
        // atribuita pentru workerii clusterului 1
        MPI_Send(vector, elements, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Send(&elements_worker, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Send(&index, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Send(&plus, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        message_log(rank, 1);

        // Primesc vectorul complet de la cluster 1
        MPI_Recv(vector, elements, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);

        // Primesc mesajele de la workerii personali si updatez rezultatul
        // final cu valorile primite
        int current_index;
        for (i = 1; i <= workers_number; i++)
        {
            int partial_vector[elements_workers[i]];
            current_index = index_workers[i];

            MPI_Recv(partial_vector, elements_workers[i], MPI_INT,
                     full_topology.rank2_workers[i], 0, MPI_COMM_WORLD, &status);

            for (int j = current_index, k = 0; k < elements_workers[i]; j++, k++)
            {
                vector[j] = partial_vector[k];
            }
        }

        // Trimit vectorul final la cluster 3
        MPI_Send(vector, elements, MPI_INT, 3, 0, MPI_COMM_WORLD);
        message_log(rank, 3);
    }
    else if (rank == 3)
    {
        ifstream cluster_file("cluster3.txt");
        string line;

        // Primesc topologia incompletata de la cluster 0
        MPI_Recv(&full_topology, 1, top, 0, 0, MPI_COMM_WORLD, &status);

        // Completez topologia cu workers pentru cluster 3
        cluster_file >> workers_number;
        full_topology.rank3_workers[0] = workers_number;
        int x;
        int i = 1;
        while (cluster_file >> x)
        {
            full_topology.rank3_workers[i] = x;
            i++;
        }
        cluster_file.close();

        // Trimit topologia incompleta la cluster 2
        MPI_Send(&full_topology, 1, top, 2, 0, MPI_COMM_WORLD);
        message_log(rank, 2);

        // Primesc topologia completata de la cluster 2 si o trimit la 0
        MPI_Recv(&full_topology, 1, top, 2, 0, MPI_COMM_WORLD, &status);
        show_topology(full_topology, rank);
        MPI_Send(&full_topology, 1, top, 0, 0, MPI_COMM_WORLD);
        message_log(rank, 0);

        for (i = 1; i <= workers_number; i++)
        {
            MPI_Send(&full_topology, 1, top,
                     full_topology.rank3_workers[i], 0, MPI_COMM_WORLD);
            message_log(rank, full_topology.rank3_workers[i]);
        }

        // Primesc vector incomplet de la cluster 0
        int vector[elements];
        MPI_Recv(vector, elements, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&elements_worker, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&index, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&plus, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        // Trimit elementele corespunzatoare fiecarui worker
        // Memorez pentru fiecare indexul de start si numarul de elemente
        int index_workers[workers_number + 1];
        int elements_workers[workers_number + 1];

        for (i = 1; i <= workers_number; i++)
        {
            int elements_this_worker = elements_worker;
            if (plus > 0)
            {
                elements_this_worker++;
                plus--;
            }

            index_workers[i] = index;
            elements_workers[i] = elements_this_worker;

            MPI_Send(&elements_this_worker, 1, MPI_INT,
                     full_topology.rank3_workers[i], 0, MPI_COMM_WORLD);
            MPI_Send(vector + index, elements_this_worker, MPI_INT,
                     full_topology.rank3_workers[i], 0, MPI_COMM_WORLD);
            message_log(rank, full_topology.rank3_workers[i]);

            index += elements_this_worker;
        }

        // Trimit vectorul la cluster 2
        // Folosesc index pentru a specifica de la ce index incepe zona
        // atribuita pentru workerii clusterului 2
        MPI_Send(vector, elements, MPI_INT, 2, 0, MPI_COMM_WORLD);
        MPI_Send(&elements_worker, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
        MPI_Send(&index, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
        MPI_Send(&plus, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
        message_log(rank, 2);

        // Primesc vectorul complet de la cluster 2
        MPI_Recv(vector, elements, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);

        // Primesc mesajele de la workerii personali si updatez rezultatul
        // final cu valorile primite
        int current_index;
        for (i = 1; i <= workers_number; i++)
        {
            int partial_vector[elements_workers[i]];
            current_index = index_workers[i];

            MPI_Recv(partial_vector, elements_workers[i], MPI_INT,
                     full_topology.rank3_workers[i], 0, MPI_COMM_WORLD, &status);

            for (int j = current_index, k = 0; k < elements_workers[i]; j++, k++)
            {
                vector[j] = partial_vector[k];
            }
        }

        // Trimit vectorul final la cluster 0
        MPI_Send(vector, elements, MPI_INT, 0, 0, MPI_COMM_WORLD);
        message_log(rank, 0);
    }
    else
    {
        // Primesc topologia de la lider
        MPI_Recv(&full_topology, 1, top, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        show_topology(full_topology, rank);

        // Primesc detaliile vectorului
        MPI_Recv(&elements_worker, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        int vector[elements_worker];
        MPI_Recv(vector, elements_worker, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        for (int i = 0; i < elements_worker; i++)
        {
            vector[i] *= 5;
        }

        // Trimit vectorul modificat
        MPI_Send(vector, elements_worker, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
        message_log(rank, status.MPI_SOURCE);
    }

    MPI_Type_free(&top);
    MPI_Finalize();
}

void show_topology(topology full_topology, int rank)
{
    cout << rank << " -> ";
    cout << "0:";
    for (int i = 1; i <= full_topology.rank0_workers[0]; i++)
    {
        if (i == full_topology.rank0_workers[0])
            cout << full_topology.rank0_workers[i] << " ";
        else
            cout << full_topology.rank0_workers[i] << ",";
    }
    cout << "1:";
    for (int i = 1; i <= full_topology.rank1_workers[0]; i++)
    {
        if (i == full_topology.rank1_workers[0])
            cout << full_topology.rank1_workers[i] << " ";
        else
            cout << full_topology.rank1_workers[i] << ",";
    }
    cout << "2:";
    for (int i = 1; i <= full_topology.rank2_workers[0]; i++)
    {
        if (i == full_topology.rank2_workers[0])
            cout << full_topology.rank2_workers[i] << " ";
        else
            cout << full_topology.rank2_workers[i] << ",";
    }
    cout << "3:";
    for (int i = 1; i <= full_topology.rank3_workers[0]; i++)
    {
        if (i == full_topology.rank3_workers[0])
            cout << full_topology.rank3_workers[i] << endl;
        else
            cout << full_topology.rank3_workers[i] << ",";
    }
}

void message_log(int source, int dest)
{
    cout << "M(" << source << "," << dest << ")" << endl;
}