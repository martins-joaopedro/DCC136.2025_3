#include <iostream>
#include <bits/stdc++.h>
using namespace std;

struct Job {
    int name;
    int start;
    int end;

    // suas dependencias: jobs que precisam ser concluídos antes para liberar o atual
    vector<int> dependencies;

    // seus dependentes: jobs que podem ser liberados se concluído o atual
    vector<int> dependents;

    // comparação feita so com o nome
    bool operator==(const Job& other) const {
        return name == other.name;
    }

};


vector<int> randomized_heuristic(map<int, vector<int>> dependencies, vector<int> delays, vector<int> LC, vector<Job> sol, int N, vector<vector<int>> S, vector<vector<int>> D, vector<int> T)
{
    // coloca na lista só os jobs possíveis de iniciar
    // para todos os dependentes, verificar se os jobs que liberam ele foram concluídos 
    for(int i=0; i<N; i++) {

        // verifica se o job i já está na solução
        if(find(sol.begin(), sol.end(), Job{i, 0, 0}) != sol.end())
            continue;
        
        // se aquele job ta sofrendo alguma restrição de delay de outro 
        if(delays[i] > 0)
            continue; 

        // se o job não tem dependencias e não está na LC
        if(dependencies[i].empty() && find(LC.begin(), LC.end(), i) == LC.end()) {
            LC.push_back(i);
            continue;
        }

        bool allDependenciesCompleted = true;
        for(auto d : dependencies[i])
            // retorna se o job não foi encontrado na lista solução
            if(find(sol.begin(), sol.end(), Job{d, 0, 0}) == sol.end()) {
                allDependenciesCompleted = false;
                break;
            }
        
        // se tem dependencias atendidas mas ainda não está na lista
        if(allDependenciesCompleted && find(LC.begin(), LC.end(), i) == LC.end())
            LC.push_back(i);
    }

    int n = LC.size();
    // peso do delay, quantidade de dependentes e setups
    int delayWeight = 1;
    int dependenciesWeight = 1;
    int setupWeight = 1;

    
    //int delays[n] = {0};
    vector<pair<int, int>> sorting; 
    
    // 1 ordena por quem oferece mais delay
    // 2 ordena por jobs que tem mais dependentes
    for(int i = 0; i < LC.size(); i++) {
        int job_id = LC[i];
        int maxDelay = 0;
        int dependents = 0;

        cout << ">>> job ID " << job_id << endl; 

        // varre a coluna -> pontua quantos jobs aquele job libera e o maior delay que ele oferece 
        for(int j = 0; j < N; j++) {
            if(D[j][job_id] != 0)
                dependents++;
            maxDelay = max(maxDelay, D[j][job_id]);
        }
        
        cout << "job: " << job_id << " " << dependents << " " << maxDelay << endl;

        int score = maxDelay * delayWeight + dependents * dependenciesWeight;
        sorting.push_back(make_pair(job_id, score));
    }

    sort(sorting.begin(), sorting.end(), 
         [](const pair<int, int>& a, const pair<int, int>& b) {
             return a.second > b.second;
         });

    vector<int> sorted;
    for(const auto& par : sorting) {
        cout << par.first << " " << par.second << endl;
        sorted.push_back(par.first);
    }

    cout << "Lista original: ";
    for(int job : LC) cout << "J" << job << " ";
    cout << endl;
    
    cout << "Lista ordenada por dependência: ";
    for(int job : sorted) cout << "J" << job << " ";
    cout << endl;

    return sorted;
}


int main() {
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
    D[3][0] = 12; 
    D[1][0] = 12; 
    D[3][2] = 8; 
    D[4][2] = 4; 
    D[4][2] = 4; 
    
    vector<int> ILC;

    // makespan 
    vector<Job> sol;

    // inicia todas as informações do job
    map<int, Job> allJobs;


    // delay que um job i está submetido por um outro job qualquer    
    vector<int> delays(N, 0);
 

    // se job i é dependente de outros k jobs
    map<int, vector<int>> dependencies;

    for(int i=0; i<N; i++)
        for(int j=0; j<N; j++)
            if(D[i][j] != 0)
                dependencies[i].push_back(j);

    for(int i=0; i<N; i++)
        for(auto v : dependencies[i])
            cout << "job " << i << " e dependente de " << v << endl;

    int currentTime = 0;

    do {
        
        ILC = randomized_heuristic(dependencies, delays, ILC, sol, N, S, D, T);
        
        // se a lista ta vazia e ainda possui jobs que não podem ser processados e ainda não acabou 
        int minTimeNeededToWait = INT_MAX;
        bool isNeededToWait = false;
        if(ILC.empty() and (N != sol.size() )) {
            for(int i=0; i<N; i++) 
                if(delays[i] != 0) {
                    minTimeNeededToWait = min(delays[i], minTimeNeededToWait);
                    isNeededToWait = true;
                }

            cout << "delay minimo pra se esperar " << minTimeNeededToWait << endl;
        }
        
        if (isNeededToWait) {
            Job idleJob{-1, currentTime, currentTime + minTimeNeededToWait}; // Job especial para tempo ocioso
            sol.push_back(idleJob);
            currentTime += minTimeNeededToWait;
            
            for(int i = 0; i < N; i++) {
                if(delays[i] > 0) {
                    delays[i] = max(0, delays[i] - minTimeNeededToWait);
                }
            }
            continue; // Volta para recalculcar ILC
        }

        int alpha = 0;
        int j = ILC[alpha];
        ILC.erase(ILC.begin() + alpha);

        //cria o job para add
        Job selectedJob;
        int startTime = currentTime;
        int completionTime;

        if(sol.empty()) {
            completionTime = startTime + T[j]; 
        } else {
            Job lastJob = sol.back();
            // Se o último job foi tempo ocioso, setup é 0
            int setup = (lastJob.name == -1) ? 0 : S[lastJob.name][j];
            completionTime = startTime + setup + T[j];
        }

        selectedJob = {j, startTime, completionTime};
        sol.push_back(selectedJob);
        currentTime = completionTime; 

        // updateDelayConstraints
        int name = selectedJob.name;
        //int startTime = selectedJob.start;
        //int endTime = selectedJob.end;
        int duration = completionTime - startTime;


        //preciso falar que o tempo ta passando -> delays
        for(int i=0; i<N; i++) {
            delays[i] = max(0, (delays[i] - duration));
        }

        for (int j=0; j<N; j++) {
            // se existe uma restrição maior daquele job para um outro aplico 
            if(D[j][name] != 0) {
                delays[j] = max(delays[j], D[j][name]);
            }
        }

        cout << "Restrições de delay ";
        for(int i=0; i<N; i++) cout << "J" << i << " " << delays[i] << endl;
        cout << endl;


        cout << "Lista solucao ";
        for(Job job : sol) cout << "J" << job.name << " " << job.start << " " << job.end << " ";
        cout << endl;


        

    } while (!ILC.empty() || N != sol.size());

    // sol deve ser uma fila (ou map) de pair que é o job e suas informações de makespan
    // usar isso para verificar se o job x pode ser add para ser feito

    return 0;
}