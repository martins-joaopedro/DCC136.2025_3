#include <iostream>
#include <bits/stdc++.h>
using namespace std;

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


int main() {

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
}