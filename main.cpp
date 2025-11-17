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
        if(delays[i] != 0)
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

    // se a lista ta vazia e ainda possui jobs que não podem ser processados e ainda não acabou 
    int minTimeNeededToWait;
    if(LC.empty() and (N != LC.size() + sol.size())) {
        for(int i=0; i<N; i++) 
            if(delays[i] != 0)
                minTimeNeededToWait = delays[i];

        for(int i=0; i<N; i++) 
            if(delays[i] != 0)
                minTimeNeededToWait = min(delays[i], minTimeNeededToWait);
        Job lastJob = sol[sol.size() -1];
        sol.push_back(Job{0, lastJob.end, lastJob.end + minTimeNeededToWait});
    }


    int n = LC.size();
    // peso do delay, quantidade de dependentes e setups
    int delayWeight = 1;
    int dependenciesWeight = 1;
    int setupWeight = 1;

    int scores[n] = {0};
    //int delays[n] = {0};
    vector<pair<int, int>> sorting; 
    
    // 1 ordena por quem oferece mais delay
    // 2 ordena por jobs que tem mais dependentes
    for(int i = 0; i < LC.size(); i++) {
        int job_id = LC[i];
        int maxDelay = 0;
        int dependents = 0;

        // varre a coluna -> pontua quantos jobs aquele job libera e o maior delay que ele oferece 
        for(int j = 0; j < N; j++) {
            if(D[j][job_id] != 0)
                dependents++;
            maxDelay = max(maxDelay, D[j][job_id]);
            // como que eu vou atribuir o valor de delay + setup + process aqui pra saber se um job pode ser executado?
            //delays[j] = maxDelay;
        }
        
        cout << "job: " << job_id << " " << dependents << " " << maxDelay << endl;

        scores[job_id] += maxDelay * delayWeight;
        scores[job_id] += dependents * dependenciesWeight;

        sorting.push_back(make_pair(job_id, scores[job_id]));
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
    D[3][0] = 3; 
    D[3][2] = 8; 
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

    do {
        int alpha = 0;
        ILC = randomized_heuristic(dependencies, delays, ILC, sol, N, S, D, T);
        int j = ILC[alpha];

        //cria o job para add
        Job selectedJob;
        if(sol.empty()) {
            int completitionTime = T[j];
            selectedJob = {j, 0, completitionTime};
            
        } else {
            // TODO: e se tiver que pagar algum delay???? ainda nao faz isso!!!
            Job lastJobCompleted = sol[sol.size() - 1]; 
            // começa no final do ultimo job 
            int startTime = lastJobCompleted.end;
            int completitionTime = startTime + S[lastJobCompleted.name][j] + T[j];
            selectedJob = {j, startTime, completitionTime};
        }

        sol.push_back(selectedJob);

        // updateDelayConstraints
        int name = selectedJob.name;
        int startTime = selectedJob.start;
        int endTime = selectedJob.end;
        int duration = endTime - startTime;


        //preciso falar que o tempo ta passando -> delays
        for(int i=0; i<N; i++) {
            cout << "passou se " << duration << " tempos" << endl; 
            int count = max(0, (delays[i] - duration));
            cout << "Conta " << count << endl; 
            delays[i] = count;
        }

        for (int j=0; j<N; j++) {
            // se existe uma restrição maior daquele job para um outro aplico 
            if(D[j][name] != 0) {
                // aplico a restrição máxima
                cout << "outra rest " << D[j][name] << endl;
                delays[j] = max(delays[j], D[j][name]);
            }
        }

        cout << "Restrições de delay ";
        for(int i=0; i<N; i++) cout << "J" << i << " " << delays[i] << endl;
        cout << endl;


        cout << "Lista solucao ";
        for(Job job : sol) cout << "J" << job.name << " " << job.start << " " << job.end << " ";
        cout << endl;


        ILC.erase(ILC.begin() + alpha);
    } while (!ILC.empty());

    // sol deve ser uma fila (ou map) de pair que é o job e suas informações de makespan
    // usar isso para verificar se o job x pode ser add para ser feito

    return 0;
}