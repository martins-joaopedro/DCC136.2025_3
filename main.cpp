#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <bits/stdc++.h>
using namespace std;

struct Job
{
    int id;
    int start;
    int end;

    // suas dependencias: jobs que precisam ser concluídos antes para liberar o atual
    vector<int> dependencies;

    // seus dependentes: jobs que podem ser liberados se concluído o atual
    vector<int> dependents;

    // comparação feita so com o nome
    bool operator==(const Job &other) const
    {
        return id == other.id;
    }
};

struct Instance
{
    int N_size;
    vector<int> processing_time;
    vector<vector<int>> setup_time;
    vector<vector<int>> delay_time;
};

Instance readDataFromFile(const string &filename);
vector<Job> randomized_greedy(int N, float alpha, map<int, Job> allJobs, vector<vector<int>> S, vector<vector<int>> D, vector<int> T);
vector<Job> randomized_heuristic(map<int, Job> allJobs, vector<int> delays, vector<Job> LC, vector<Job> sol, int N, vector<vector<int>> S, vector<vector<int>> D, vector<int> T);
vector<Job> local_search(int N, vector<Job> initialSolution, map<int, Job> allJobs, vector<vector<int>> S, vector<vector<int>> D, vector<int> T);
bool validate_solution(vector<Job> sol, map<int, Job> allJobs, vector<vector<int>> D, vector<int> T, vector<vector<int>> S);
int calculate_makespan(vector<Job> sol);
vector<Job> recalculate_schedule(vector<Job> jobSequence, map<int, Job> allJobs, vector<vector<int>> S, vector<vector<int>> D, vector<int> T);

Instance readDataFromFile(const string &filename)
{
    Instance instance;
    ifstream inputFile(filename);

    if (!inputFile.is_open())
    {
        cerr << "Erro: Nao pode abrir arquivo " << filename << endl;
        return instance;
    }

    // le o numero de jobs N
    string line;
    if (getline(inputFile, line))
    {
        if (line.find("R=") == 0)
        {
            instance.N_size = stoi(line.substr(2));
        }
        else
        {
            cerr << "Erro: Nao encontrou parametro N." << endl;
            inputFile.close();
            return instance;
        }
    }

    // confere se e um tamanho valido
    if (instance.N_size <= 0)
    {
        cerr << "Erro: N precisa ser positivo " << instance.N_size << endl;
        inputFile.close();
        return instance;
    }

    const int N = instance.N_size;
    instance.processing_time.resize(N);

    // le tempos de
    if (getline(inputFile, line))
    {
        if (line.find("Pi=(") == 0 && line.back() == ')')
        {
            // remove "Pi=(" e ")"
            string values_str = line.substr(4, line.length() - 5); 
            stringstream ss(values_str);
            string token;
            instance.processing_time.resize(N);
            int idx = 0;

            while (getline(ss, token, ','))
            {
                if (idx < N)
                {
                    instance.processing_time[idx++] = stoi(token);
                }
            }
        }
        else
        {
            cerr << "Error: Nao pode ler os tempos de processamento." << endl;
            inputFile.close();
            return instance;
        }

        // pula a linha "A="
        if (!getline(inputFile, line) || line != "A=")
        {
            cerr << "Error: Expected 'A=' line." << endl;
            inputFile.close();
            return instance;
        }

        // processa matriz de delays
        instance.delay_time.resize(N, vector<int>(N, -1));
        while (getline(inputFile, line))
        {
            // para quando encontrar "Sij="
            if (line == "Sij=")
                break;
            

            // ignora linhas vazias
            if (line.empty())
                continue;

            // linha de restrição: i,j,valor
            stringstream ss(line);
            string token;
            vector<int> values;

            while (getline(ss, token, ','))            
                values.push_back(stoi(token));

            if (values.size() == 3)
            {
                //TODO: ta zero based?
                // -> não, então eu converti pra zero based aqui já que a aplicação toda está
                int i = values[0] - 1; 
                int j = values[1] - 1; 
                int delay = values[2];

                // RESTRIÇÃO É DE I PARA J -> JOB J DEPENDE DE JOB 1 TERMINAR ANTES E ESPERAR O DELAY
                if (i >= 0 && i < N && j >= 0 && j < N)
                    instance.delay_time[i][j] = delay;
                else
                    cerr << "Erro: Restricao invalida " << line << endl; 
            }
            else
                cerr << "Erro: Formato invalido: " << line << endl;
            
        }

        // processa matriz de setups
        instance.setup_time.resize(N, vector<int>(N));
        int row = 0;
        while (getline(inputFile, line) && row < N)
        {
            // ignora linhas vazias
            if (line.empty())
                continue;

            stringstream ss(line);
            string token;
            int col = 0;

            while (getline(ss, token, ',') && col < N)
            {
                instance.setup_time[row][col] = stoi(token);
                col++;
            }

            if (col != N)
            {
                cerr << "Error: Expected " << N << " values in setup matrix row " << row + 1 << ", but found " << col << endl;
                inputFile.close();
                return instance;
            }

            row++;
        }

        if (row != N)
        {
            cerr << "Error: Expected " << N << " rows in setup matrix, but found " << row << endl;
            inputFile.close();
            return instance;
        }

        inputFile.close();
        return instance;
    }
}

vector<Job> randomized_heuristic(map<int, Job> allJobs, vector<int> delays, vector<Job> LC, vector<Job> sol, int N, vector<vector<int>> S, vector<vector<int>> D, vector<int> T)
{
    // coloca na lista só os jobs possíveis de iniciar
    // para todos os dependentes, verificar se os jobs que liberam ele foram concluídos
    for (auto it = allJobs.begin(); it != allJobs.end(); ++it)
    {

        int id = it->first;
        Job job = it->second;

        // verifica se o job i já está na solução
        if (find(sol.begin(), sol.end(), job) != sol.end())
            continue;

        // se aquele job ta sofrendo alguma restrição de delay de outro
        if (delays[id] > 0)
            continue;

        // se o job não tem dependencias e não está na LC
        if (job.dependencies.empty() && find(LC.begin(), LC.end(), job) == LC.end())
        {
            LC.push_back(job);
            continue;
        }

        bool allDependenciesCompleted = true;
        for (auto dep : job.dependencies)
            // retorna se o job não foi encontrado na lista solução
            // compara com um job fas
            if (find(sol.begin(), sol.end(), allJobs[dep]) == sol.end())
            {
                allDependenciesCompleted = false;
                break;
            }

        // se tem dependencias atendidas mas ainda não está na lista
        if (allDependenciesCompleted && find(LC.begin(), LC.end(), job) == LC.end())
            LC.push_back(job);
    }

    int n = LC.size();

    float delayWeight = 0.1;        // Prioridade dos adiantar quem poderia travar
    float dependenciesWeight = 0.3; // Prioridade em liberar outros jobs
    float savingWeight = 0.5;       // Prioridade em realizar economia em relação aos outros candidatos
    float setupWeight = 2;          // Prioridade em menor setup em relação ao job anterior

    vector<pair<int, int>> sorting;

    for (auto job : LC) {
        int job_id = job.id;
        int maxDelay = 0;
        float dependents = job.dependents.size();
        float saving = 0;
        float setupFromPrevious = 0;

        // pontua o maior delay que um job oferece
        for (int j = 0; j < N; j++)
            maxDelay = max(maxDelay, D[job_id][j]);

        // setup em relação ao job anterior
        if (!sol.empty()) {
            int lastJobId = sol.back().id;
            setupFromPrevious = S[lastJobId][job_id];
        }

        // calculo potencial economia
        for(Job jobX : LC) {
            if(job_id == jobX.id)
                continue;

            int directCost = S[job_id][jobX.id];    // job atual -> job X
            int inverseCost = S[jobX.id][job_id];   // job X -> job atual
            // Economia se job atual vier ANTES do job X
            // Valor positivo = economiza se vier antes
            // Valor negativo = economiza se vier depois
            saving += (inverseCost - directCost);
        }

        cout << "\n\n__________SCORE JOB " << job_id << "__________" << endl;
        cout << ">>> DELAY: " << maxDelay << endl;
        cout << ">>> DEPENDENTES: " << dependents << endl;
        cout << ">>> SETUP DO ANTERIOR: " << setupFromPrevious << endl;
        cout << ">>> ECONOMIA MÉDIA: " << saving << endl;

        float score = maxDelay * delayWeight
            + dependents * dependenciesWeight
            + saving * savingWeight
            - setupFromPrevious * setupWeight; // subtrai pq menor setup -> mais a frente  
        
        cout << ">>>>> SCORE FINAL: " << score << endl;
        sorting.push_back(make_pair(job_id, score));
    }

    sort(sorting.begin(), sorting.end(),
         [](const pair<int, float> &a, const pair<int, float> &b)
         {
             return a.second > b.second;
         });

    vector<Job> sorted;
    for (const auto &par : sorting)
        sorted.push_back(allJobs[par.first]);
    
    // melhor heurística que so funciona a partir do primeiro candidato na sol
    // procura menor setup em relação ao anterior 🤡🤡🤡🤡🤡
    if (!sol.empty()) {
        Job lastJob = sol.back();
        sort(sorted.begin(), sorted.end(), 
            [&](const Job& a, const Job& b) {
                return S[lastJob.id][a.id] < S[lastJob.id][b.id];
            });
    }

    return sorted;
}

vector<Job> randomized_greedy(int N, float alpha, map<int, Job> allJobs, vector<vector<int>> S, vector<vector<int>> D, vector<int> T)
{

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
        for (auto job : LC)
            cout << "J" << job.id << " ";
        cout << endl;

        // se a lista ta vazia e ainda possui jobs que não podem ser processados e ainda não acabou
        int minTimeNeededToWait = INT_MAX;
        bool isNeededToWait = false;
        if (LC.empty() and (N != sol.size() - stallTimes))
        {
            for (int i = 0; i < N; i++)
                if (delays[i] != 0)
                {
                    minTimeNeededToWait = min(delays[i], minTimeNeededToWait);
                    isNeededToWait = true;
                }
            cout << ">>> PRECISA ESPERAR UM TEMPO DE " << minTimeNeededToWait << endl;
        }

        if (isNeededToWait)
        {
            Job idleJob{-1, currentTime, currentTime + minTimeNeededToWait}; // job especial para tempo ocioso
            sol.push_back(idleJob);
            currentTime += minTimeNeededToWait;
            stallTimes++;

            for (int i = 0; i < N; i++)
            {
                if (delays[i] > 0)
                {
                    delays[i] = max(0, delays[i] - minTimeNeededToWait);
                }
            }
            continue;
        }

        //
        int k = max(1, (int)(alpha * LC.size()));
        int random_index = rand() % k;
        Job selectedJob = LC[random_index];
        cout << "\n[ JOB SELECIONADO: " << selectedJob.id << " ] ( " << random_index << " )" << endl
             << endl;
        ;
        LC.erase(LC.begin() + random_index);

        int startTime = currentTime;
        int completionTime;

        if (sol.empty())
        {
            completionTime = startTime + T[selectedJob.id];
        }
        else
        {
            Job lastJob = sol.back();
            // se o último job foi tempo ocioso, setup é o do anterior ao stall
            // TODO: incluir setup dentro do tempo de delay
            int setup;
            if (lastJob.id == -1)
            {
                Job lastValidJob = sol[sol.size() - 2];
                setup = S[lastValidJob.id][selectedJob.id];
            }
            else
                setup = S[lastJob.id][selectedJob.id];
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
        for (int i = 0; i < N; i++)
        {
            delays[i] = max(0, (delays[i] - duration));
        }

        for (int j = 0; j < N; j++)
        {
            // se existe uma restrição maior daquele job para um outro aplico
            if (D[j][selectedJob.id] != 0)
            {
                delays[j] = max(delays[j], D[j][selectedJob.id]);
            }
        }

        cout << ">>> RESTRICOES DE DELAY ATUAIS: " << endl;
        for (int i = 0; i < N; i++)
            cout << "J" << i << " " << delays[i] << " | ";
        cout << endl;

    } while (N != (sol.size() - stallTimes));

    return sol;
}

bool validate_solution(vector<Job> sol, map<int, Job> allJobs, vector<vector<int>> D, vector<int> T, vector<vector<int>> S) {
    // cria cópia da lista de jobs
    vector<Job> actualJobs;
    for (auto job : sol) {
        if (job.id != -1) {
            actualJobs.push_back(job);
        }
    }
    
    int N = actualJobs.size();
    
    // Confere precedências
    for (int i = 0; i < N; i++) {
        Job currentJob = actualJobs[i];
        
        // Checa tds as dependencias
        for (int dep : currentJob.dependencies) {
            // Seleciona a dependencia do job atual
            bool found = false;
            int depEndTime = 0;
            
            for (int j = 0; j < i; j++) {
                if (actualJobs[j].id == dep) {
                    found = true;
                    depEndTime = actualJobs[j].end;
                    break;
                }
            }
            
            if (!found) {
                cout << "ERRO! Job " << currentJob.id << " depende do job " << dep << " mas a dependencia nao foi atendida" << endl;
                cout << "Solução descartada!" << endl;
                return false;
            }
            
            // Confere se delay foi satisfeito
            int requiredDelay = D[dep][currentJob.id];
            if (requiredDelay > 0) {
                int actualGap = currentJob.start - depEndTime;
                if (actualGap < requiredDelay) {                
                    cout << "Erro no delay! Job " << currentJob.id 
                         << " deveria iniciar a partir de " << requiredDelay 
                         << " instantes apos o termino do job " << dep << ", mas o gap e de " << actualGap << endl;
                    cout << "Solução descartada!" << endl;
                    return false;
                }
            }
        }
    }
    
    // Confere processamento e setup
    for (int i = 0; i < N; i++) {
        Job currentJob = actualJobs[i];
        
        // Confere processamento
        int expectedDuration = T[currentJob.id];
        int actualDuration = currentJob.end - currentJob.start;
        
        if (actualDuration != expectedDuration) {
            // Confere se setup incluido
            if (i > 0) {
                Job previousJob = actualJobs[i-1];
                int setupTime = S[previousJob.id][currentJob.id];
                if (actualDuration != expectedDuration + setupTime) {
                    cout << "Erro no tempo de processamento! Job " << currentJob.id 
                         << " deveria ter duracao de " << expectedDuration 
                         << " + setup, mas o tempo real foi de " << actualDuration << endl;
                    cout << "Solução descartada!" << endl;
                    return false;
                }
            } else {
                if (actualDuration != expectedDuration) {
                    cout << "Erro no tempo de processamento! Job " << currentJob.id 
                         << " deveria ter duracao de " << expectedDuration 
                         << ", mas o tempo real foi de " << actualDuration << endl;
                    cout << "Solução descartada!" << endl;
                    return false;
                }
            }
        }
        
        // Check for overlaps ???
        if (i > 0) {
            Job previousJob = actualJobs[i-1];
            if (previousJob.end > currentJob.start) {
                // cerr << "Error: Jobs " << previousJob.id << " and " << currentJob.id << " overlap" << endl;
                return false;
            }
        }
    }
    
    return true;
}

int calculate_makespan(vector<Job> sol) {
    if (sol.empty()) return 0;
    
    int makespan = 0;
    for (auto job : sol) {
        makespan = max(makespan, job.end);
    }
    return makespan;
}

vector<Job> recalculate_schedule(vector<Job> jobSequence, map<int, Job> allJobs, vector<vector<int>> S, vector<vector<int>> D, vector<int> T) {
    vector<Job> newSchedule;
    int currentTime = 0;
    
    for (int i = 0; i < jobSequence.size(); i++) {
        Job currentJob = jobSequence[i];
        currentJob.start = currentTime;
        
        // Adiciona setup se n for o 1° job
        if (i > 0) {
            Job previousJob = newSchedule.back();
            currentJob.start += S[previousJob.id][currentJob.id];
        }
        
        // Confere delay
        for (int dep : currentJob.dependencies) {
            // Encontra qd a dependencia acaba
            for (const Job& scheduledJob : newSchedule) {
                if (scheduledJob.id == dep) {
                    int requiredStartTime = scheduledJob.end + D[dep][currentJob.id];
                    if (currentJob.start < requiredStartTime) {
                        // adiciona tempo ocioso se necessario
                        currentJob.start = requiredStartTime;
                    }
                    break;
                }
            }
        }
        
        currentJob.end = currentJob.start + T[currentJob.id];
        newSchedule.push_back(currentJob);
        currentTime = currentJob.end;
    }
    
    return newSchedule;
}

vector<Job> local_search(int N, vector<Job> initialSolution, map<int, Job> allJobs, vector<vector<int>> S, vector<vector<int>> D, vector<int> T) {
    // Remove idle jobs for local search operations
    vector<Job> actualJobs;
    for (auto job : initialSolution) {
        if (job.id != -1) {
            actualJobs.push_back(job);
        }
    }
    
    vector<Job> bestSol = recalculate_schedule(actualJobs, allJobs, S, D, T);
    int bestMakespan = calculate_makespan(bestSol);
    bool improved = true;
    int iterations = 0;
    const int MAX_ITERATIONS = 100;
    
    cout << "Makespan Inicial: " << bestMakespan << endl;
    
    while (improved && iterations < MAX_ITERATIONS) {
        improved = false;
        
        // Tenta combinações entre tds os jobs
        for (int i = 0; i < actualJobs.size() - 1; i++) {
            vector<Job> candidate = actualJobs;
            
            // Troca jobs adjacentes
            swap(candidate[i], candidate[i + 1]);
            
            // Atualiza os tempos
            vector<Job> newSchedule = recalculate_schedule(candidate, allJobs, S, D, T);
            
            // Valida e confere makespan
            if (validate_solution(newSchedule, allJobs, D, T, S)) {
                int newMakespan = calculate_makespan(newSchedule);
                
                if (newMakespan < bestMakespan) {
                    cout << "Melhora encontrada! Makespan anterior: " << bestMakespan 
                         << ", makespan novo: " << newMakespan 
                         << " (troca posições " << i << " e " << i + 1 << ")" << endl;
                    
                    bestSol = newSchedule;
                    bestMakespan = newMakespan;
                    actualJobs = candidate;
                    improved = true;
                    break; // Recomeça a busca com nova solucao
                }
            }
        }
        
        // Se não melhorar com troca, tenta inserção
        if (!improved) {
            for (int i = 0; i < actualJobs.size(); i++) {
                for (int j = 0; j < actualJobs.size(); j++) {
                    if (i == j) continue;
                    
                    vector<Job> candidate = actualJobs;
                    Job jobToMove = candidate[i];
                    candidate.erase(candidate.begin() + i);
                    
                    if (j > i) {
                        candidate.insert(candidate.begin() + (j - 1), jobToMove);
                    } else {
                        candidate.insert(candidate.begin() + j, jobToMove);
                    }
                    
                    // Recalculate the entire schedule
                    vector<Job> newSchedule = recalculate_schedule(candidate, allJobs, S, D, T);
                    
                    // Validate and check makespan
                    if (validate_solution(newSchedule, allJobs, D, T, S)) {
                        int newMakespan = calculate_makespan(newSchedule);
                        
                        if (newMakespan < bestMakespan) {
                            cout << "Melhora encontrada! Makespan anterior: " << bestMakespan 
                                 << ", makespan novo: " << newMakespan 
                                 << " (move job de " << i << " para " << j << ")" << endl;
                            
                            bestSol = newSchedule;
                            bestMakespan = newMakespan;
                            actualJobs = candidate;
                            improved = true;
                            break;
                        }
                    }
                }
                if (improved) break;
            }
        }
        
        iterations++;
    }
    
    cout << "Terminou Busca Local. Melhor makespan: " << bestMakespan << endl;
    return bestSol;
}

void calculaSetupETempoOcioso(vector<Job> sol, vector<vector<int>> S, vector<vector<int>> D) {
    int totalSetup = 0;
    int totalOcioso = 0;

    for (int i = 1; i < sol.size(); i++) {
        Job prevJob = sol[i - 1];
        Job currJob = sol[i];

        if (prevJob.id != -1 && currJob.id != -1) {
            totalSetup += S[prevJob.id][currJob.id];
            if (currJob.start > prevJob.end && S[prevJob.id][currJob.id] < (currJob.start - prevJob.end)) {
                totalOcioso += (currJob.start - prevJob.end) - S[prevJob.id][currJob.id];
            }
            
        }
    }

    cout << "Total Setup Pago: " << totalSetup << endl;
    cout << "Tempo Ocioso Pago: " << totalOcioso << endl;
}

int main(int argc, char *argv[]) {
    string filename = argv[1];

    Instance result = readDataFromFile(filename);

    if (result.N_size > 0)
    {
        const int N = result.N_size;

        cout << "Numero Jobs N: " << N << endl;

        cout << "Processing Time (" << N << " total): ";
        for (size_t i = 0; i < result.processing_time.size(); ++i)
        {
            cout << result.processing_time[i] << (i == result.processing_time.size() - 1 ? "" : ", ");
        }
        cout << endl;

        // Print Matrix 1
        cout << "\nMatriz Setup (" << N << "x" << N << "):" << endl;
        for (const auto &row : result.setup_time)
        {
            for (size_t i = 0; i < row.size(); ++i)
            {
                cout << row[i] << (i == row.size() - 1 ? "" : "\t"); // Using tab for alignment
            }
            cout << endl;
        }

        // Print Matrix 2
        cout << "\nMatriz Delay (" << N << "x" << N << "):" << endl;
        for (const auto &row : result.delay_time)
        {
            for (size_t i = 0; i < row.size(); ++i)
            {
                cout << row[i] << (i == row.size() - 1 ? "" : "\t");
            }
            cout << endl;
        }

        int noDependentConstraintValue = -1;

        // inicia todas as informações dos jobs
        map<int, Job> allJobs;
        for (int i = 0; i < N; i++)
        {
            Job job;
            job.id = i;
            job.start = 0;
            job.end = 0;
            for (int j = 0; j < N; j++)
            {
                // DEPENDENCIA: DE QUEM O JOB I DEPENDE ( J -> I, JOB I DEPENDE DO J)
                if (result.delay_time[j][i] > noDependentConstraintValue)
                    job.dependencies.push_back(j);

                // DEPENDENTES: QUEM O JOB I LIBERA ( I -> J, JOB J É DEPENDENTE DE I)
                if (result.delay_time[i][j] > noDependentConstraintValue)
                    job.dependents.push_back(j);
            }
            allJobs[i] = job;
        }

        for (auto it = allJobs.begin(); it != allJobs.end(); ++it)
        {
            int id = it->first;
            Job job = it->second;
            cout << "Job " << id << " -> " << job.dependencies.size() << " - " << job.dependents.size() << endl;
        }

        vector<float> alphas = {0.1, 0.3, 0.5};
        for(auto alpha : alphas) {

            // makespan
            vector<Job> sol = randomized_greedy(N, alpha, allJobs, result.setup_time, result.delay_time, result.processing_time);
    
            cout << "\n\nSOLUCAO FINAL:" << endl;
            for (Job job : sol)
            {
                if (job.id == -1)
                {
                    cout << "IDLE: " << job.start << " - " << job.end << endl;
                }
                else
                {
                    cout << "J" << job.id+1 << ": " << job.start << " - " << job.end << endl;
                }
            }

            // Validate the solution
            cout << "\nValidando Solucao..." << endl;
            if (validate_solution(sol, allJobs, result.delay_time, result.processing_time, result.setup_time)) {
                cout << "Solução é valida" << endl;
            } else {
                cout << "Solução é invalida" << endl;
            }

            int initialMakespan = calculate_makespan(sol);
            cout << "Makespan inicial: " << initialMakespan << endl;

            // Apply local search
            cout << "\nAplicando busca local..." << endl;
            vector<Job> improvedSol = local_search(N, sol, allJobs, result.setup_time, result.delay_time, result.processing_time);
            
            cout << "\nSolução melhorada:" << endl;
            for (Job job : improvedSol)
            {
                cout << "J" << job.id+1 << ": " << job.start << " - " << job.end << endl;
            }

            int finalMakespan = calculate_makespan(improvedSol);
            cout << "Makespan final: " << finalMakespan << endl;
            cout << "Melhora: " << (initialMakespan - finalMakespan) << " unidades" << endl;

            calculaSetupETempoOcioso(improvedSol, result.setup_time, result.delay_time);

            system("pause");
        }

    }
    else
    {
        cout << "File could not be parsed successfully (N was 0 or file could not be opened)." << endl;
    }

    return 0;
}