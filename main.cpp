#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <bits/stdc++.h>
using namespace std;

struct Grafo
{
    int N_size;

    vector<int> processing_time;

    vector<vector<int>> setup_time;
    vector<vector<int>> delay_time;
};

/**
 * @brief Reads and parses infos from a text file following the generalized structure:
 * (1st value = N, then N individual values, then two NxN matrices).
 *
 * @param filename The path to the text file to read.
 * @return Grafo A struct containing the parsed size, N values, and matrices.
 */
Grafo readDataFromFile(const string& filename)
{
    Grafo infos;
    ifstream inputFile(filename);

    if (!inputFile.is_open())
    {
        cerr << "Error: Could not open file " << filename << endl;
        return infos;
    }

    // 1. Le o numero de jobs N
    if (!(inputFile >> infos.N_size))
    {
        cerr << "Error: Could not read the size parameter N." << endl;
        inputFile.close();
        return infos;
    }
    
    // Confere se e um tamanho valido
    if (infos.N_size <= 0) {
        cerr << "Error: N must be a positive integer. Read N = " << infos.N_size << endl;
        inputFile.close();
        return infos;
    }

    const int N = infos.N_size;

    // 2. Le os N processing_times
    infos.processing_time.resize(N);
    for (int i = 0; i < N; ++i)
    {
        if (!(inputFile >> infos.processing_time[i]))
        {
            cerr << "Error: Could not read individual value #" << i + 1 << " out of " << N << "." << endl;
            inputFile.close();
            return infos;
        }
    }

    // 3. Auxiliar para ler as matrizes
    auto read_matrix = [&](vector<vector<int>>& matrix, const string& matrix_name) {
        matrix.resize(N, vector<int>(N));
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                if (!(inputFile >> matrix[i][j]))
                {
                    cerr << "Error: Could not read " << matrix_name << " element at (" << i + 1 << ", " << j + 1 << "). File ended prematurely." << endl;
                    return false; 
                }
            }
        }
        return true; 
    };

    // 4. Le as 2 matrizes
    if (!read_matrix(infos.setup_time, "Matriz Setup")) {
        inputFile.close();
        return infos;
    }
    
    if (!read_matrix(infos.delay_time, "Matrix Delay")) {
        inputFile.close();
        return infos;
    }


    inputFile.close();
    return infos;
}

struct Job {
    int id;
    int start;
    int end;

    // suas dependencias: jobs que precisam ser concluídos antes para liberar o atual
    vector<int> dependencies;

    // seus dependentes: jobs que podem ser liberados se concluído o atual
    vector<int> dependents;

    // comparação feita so com o nome
    bool operator==(const Job& other) const {
        return id == other.id;
    }
};

vector<Job> randomized_greedy(int N, float alpha, map<int, Job> allJobs, vector<vector<int>> S, vector<vector<int>> D, vector<int> T);
vector<Job> randomized_heuristic(map<int, Job> allJobs, vector<int> delays, vector<Job> LC, vector<Job> sol, int N, vector<vector<int>> S, vector<vector<int>> D, vector<int> T);

vector<Job> randomized_heuristic(map<int, Job> allJobs, vector<int> delays, vector<Job> LC, vector<Job> sol, int N, vector<vector<int>> S, vector<vector<int>> D, vector<int> T) {
    // coloca na lista só os jobs possíveis de iniciar
    // para todos os dependentes, verificar se os jobs que liberam ele foram concluídos 
    for (auto it = allJobs.begin(); it != allJobs.end(); ++it) {

        int id = it->first;
        Job job = it->second;

        // verifica se o job i já está na solução
        if(find(sol.begin(), sol.end(), job) != sol.end())
            continue;
        
        // se aquele job ta sofrendo alguma restrição de delay de outro 
        if(delays[id] > 0)
            continue; 

        // se o job não tem dependencias e não está na LC
        if(job.dependencies.empty() && find(LC.begin(), LC.end(), job) == LC.end()) {
            LC.push_back(job);
            continue;
        }

        bool allDependenciesCompleted = true;
        for(auto dep : job.dependencies)
            // retorna se o job não foi encontrado na lista solução
            // compara com um job fas
            if(find(sol.begin(), sol.end(), allJobs[dep]) == sol.end()) {
                allDependenciesCompleted = false;
                break;
            }
        
        // se tem dependencias atendidas mas ainda não está na lista
        if(allDependenciesCompleted && find(LC.begin(), LC.end(), job) == LC.end())
            LC.push_back(job);
    }

    int n = LC.size();
    // peso do delay, quantidade de dependentes e setups
    int delayWeight = 1;
    int dependenciesWeight = 1;
    int setupWeight = -2;

    vector<pair<int, int>> sorting; 
    
    for(auto job : LC) {
        int job_id = job.id;
        int maxDelay = 0;
        int dependents = 0;
        int setup = 0;

        // pontua quantos jobs aquele job libera (coluna) e o maior delay que ele oferece 
        for(int j = 0; j < N; j++) {
            if(D[j][job_id] != 0)
                dependents++;
            maxDelay = max(maxDelay, D[j][job_id]);
        }
        
        // vejo em relação aos outros jobs o quanto economiza passando por ele
        for(auto jobI : LC) {
            for(auto jobJ : LC) {
                if(job_id == jobI.id || job_id == jobJ.id || jobI.id == jobJ.id) 
                    continue;
                int directCost = S[jobI.id][jobJ.id];
                int viaJobCost = S[jobI.id][job_id] + S[job_id][jobJ.id];
                cout << "\n>>> TESTANDO CAMINHOS PELO JOB " << job_id << endl;
                cout << "DIRETO DO JOB I " << jobI.id << " PARA O JOB J " << jobJ.id << " => " << directCost << endl;
                cout << "ATALHO DE I -> J PASSANDO PELO JOB " << job_id << " => " << viaJobCost << endl;
                setup += (directCost - viaJobCost);
            }
        }

        // pontuação ajustável para experimentar o que impacta mais nas soluções
        cout << "\n\n__________SCORE JOB " << job_id << "__________" << endl;
        cout << ">>> DELAY " << maxDelay << endl;
        cout << ">>> DEPENDENTES " << dependents << endl;
        cout << ">>> SETUP " << setup << endl;
        int score = maxDelay * delayWeight + dependents * dependenciesWeight + setup * setupWeight;
        cout << ">>>>> SCORE FINAL: " << score << endl;
        sorting.push_back(make_pair(job_id, score));
    }

    sort(sorting.begin(), sorting.end(), 
         [](const pair<int, int>& a, const pair<int, int>& b) {
             return a.second > b.second;
         });

    vector<Job> sorted;
    for(const auto& par : sorting) 
        sorted.push_back(allJobs[par.first]);
    return sorted;
}


vector<Job> randomized_greedy(int N, float alpha, map<int, Job> allJobs, vector<vector<int>> S, vector<vector<int>> D, vector<int> T) {

    // delay que um job i está submetido por um outro job qualquer    
    vector<int> delays(N, 0);
 
    int currentTime = 0;
    int stallTimes = 0;
    srand(static_cast<unsigned>(time(0)));

    vector<Job> LC;
    vector<Job> sol;

    do {
        
        LC = randomized_heuristic(allJobs, delays, LC, sol, N, S, D, T);
        
        cout << "\n\n>>> LISTA ORDENADA: ";
        for(auto job : LC) cout << "J" << job.id << " ";
            cout << endl;
        
        // se a lista ta vazia e ainda possui jobs que não podem ser processados e ainda não acabou 
        int minTimeNeededToWait = INT_MAX;
        bool isNeededToWait = false;
        if(LC.empty() and (N != sol.size() - stallTimes )) {
            for(int i=0; i<N; i++) 
                if(delays[i] != 0) {
                    minTimeNeededToWait = min(delays[i], minTimeNeededToWait);
                    isNeededToWait = true;
                }
            cout << ">>> PRECISA ESPERAR UM TEMPO DE " << minTimeNeededToWait << endl;
        }
        
        if (isNeededToWait) {
            Job idleJob{-1, currentTime, currentTime + minTimeNeededToWait}; // job especial para tempo ocioso
            sol.push_back(idleJob);
            currentTime += minTimeNeededToWait;
            stallTimes++;
            
            for(int i = 0; i < N; i++) {
                if(delays[i] > 0) {
                    delays[i] = max(0, delays[i] - minTimeNeededToWait);
                }
            }
            continue; 
        }

        // 
        int k = max(1, (int)(alpha * LC.size()));  
        int random_index = rand() % k;  
        Job selectedJob = LC[random_index];
        cout << "\n[ JOB SELECIONADO: " << selectedJob.id << " ] ( " << random_index << " )" << endl << endl;;
        LC.erase(LC.begin() + random_index);

        int startTime = currentTime;
        int completionTime;

        if(sol.empty()) {
            completionTime = startTime + T[selectedJob.id]; 
        } else {
            Job lastJob = sol.back();
            // se o último job foi tempo ocioso, setup é o do anterior ao stall
            // TODO: incluir setup dentro do tempo de delay 
            int setup;
            if(lastJob.id == -1) {
                Job lastValidJob = sol[sol.size() -2];
                setup = S[lastValidJob.id][selectedJob.id]  ;
            } else setup = S[lastJob.id][selectedJob.id];
            completionTime = startTime + setup + T[selectedJob.id];
        }

        // atualizo as informações de makespan daquele momento
        selectedJob.start = startTime;
        selectedJob.end = completionTime;
        sol.push_back(selectedJob);

        // atualizo a passagem do tempo
        currentTime = completionTime; 
        int duration = completionTime - startTime;

        // atalizo os delays
        for(int i=0; i<N; i++) {
            delays[i] = max(0, (delays[i] - duration));
        }

        for (int j=0; j<N; j++) {
            // se existe uma restrição maior daquele job para um outro aplico 
            if(D[j][selectedJob.id] != 0) {
                delays[j] = max(delays[j], D[j][selectedJob.id]);
            }
        }

        cout << ">>> RESTRICOES DE DELAY ATUAIS: " << endl;
        for(int i=0; i<N; i++) 
            cout << "J" << i << " " << delays[i] << " | ";
        cout << endl;

    } while (N != (sol.size() - stallTimes));

    return sol;
}


/*int main() {

    // le as informações do job
    int N = 5;
    vector<int> T = {2, 5, 2, 1, 1};

    vector<vector<int>> S = {
        {0, 4, 5, 3, 5}, 
        {2, 0, 4, 3, 2}, 
        {5, 5, 0, 2, 2}, 
        {1, 3, 2, 0, 5},
        {4, 3, 1, 1, 0}
    };

    vector<vector<int>> D(N, vector<int>(N, 0));
    D[3][0] = 3; 
    D[3][2] = 8; 
    D[4][2] = 4; 
    
    // inicia todas as informações do job
    map<int, Job> allJobs;

    for(int i = 0; i < N; i++) {
        Job job;
        job.id = i;
        job.start = 0;
        job.end = 0;
        for(int j = 0; j < N; j++) {
            if(D[i][j] != 0)
                job.dependencies.push_back(j);
            if(D[j][i] != 0)
                job.dependents.push_back(j);
        }
        allJobs[i] = job;
    }

    // makespan 
    float alpha = 0.5;
    vector<Job> sol = randomized_greedy(N, alpha, allJobs, S, D, T);

    cout << "\n\nSOLUCAO FINAL:" << endl;
    for(Job job : sol) {
        if(job.id == -1) {
            cout << "IDLE: " << job.start << " - " << job.end << endl;
        } else {
            cout << "J" << job.id << ": " << job.start << " - " << job.end << endl;
        }
    }

    return 0;
}*/

int main(int argc, char* argv[])
{
    string filename = argv[1];
    
    Grafo result = readDataFromFile(filename);

    if (result.N_size > 0) {
        const int N = result.N_size;
        
        cout << "Numero Jobs N: " << N << endl;

        cout << "Processing Time (" << N << " total): ";
        for (size_t i = 0; i < result.processing_time.size(); ++i) {
            cout << result.processing_time[i] << (i == result.processing_time.size() - 1 ? "" : ", ");
        }
        cout << endl;

        // Print Matrix 1
        cout << "\nMatriz Setup (" << N << "x" << N << "):" << endl;
        for (const auto& row : result.setup_time) {
            for (size_t i = 0; i < row.size(); ++i) {
                cout << row[i] << (i == row.size() - 1 ? "" : "\t"); // Using tab for alignment
            }
            cout << endl;
        }

        // Print Matrix 2
        cout << "\nMatriz Delay (" << N << "x" << N << "):" << endl;
        for (const auto& row : result.delay_time) {
            for (size_t i = 0; i < row.size(); ++i) {
                cout << row[i] << (i == row.size() - 1 ? "" : "\t");
            }
            cout << endl;
        }
    } else {
        cout << "File could not be parsed successfully (N was 0 or file could not be opened)." << endl;
    }

    return 0;
}