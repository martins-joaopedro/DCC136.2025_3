#include <iostream>
#include <bits/stdc++.h>
using namespace std;

vector<int> randomized_heuristic(vector<int> LC, int N, vector<vector<int>> S, vector<vector<int>> D)
{
    vector<pair<int, int>> dependencies; 
    
    for(int i = 0; i < N; i++) {
        int job_id = LC[i];
        int acc = 0;
        
        // Contar quantos jobs dependem deste job
        for(int j = 0; j < N; j++) {
            if(D[j][job_id] != 0)
                acc++;
        }
        
        dependencies.push_back(make_pair(job_id, acc));
    }

    sort(dependencies.begin(), dependencies.end(), 
         [](const pair<int, int>& a, const pair<int, int>& b) {
             return a.second > b.second;
         });

    vector<int> lista_ordenada;
    for(const auto& par : dependencies) {
        lista_ordenada.push_back(par.first);
    }

    // Passo 4: Mostrar resultado
    cout << "Lista original: ";
    for(int job : LC) cout << "J" << job << " ";
    cout << endl;
    
    cout << "Lista ordenada por dependência: ";
    for(int job : lista_ordenada) cout << "J" << job << " ";
    cout << endl;

    return lista_ordenada;
}

int main() {
    int N = 4;

    vector<vector<int>> S = {
        {0, 2, 4, 1}, 
        {3, 0, 2, 5}, 
        {1, 4, 3, 0}, 
        {2, 1, 3, 0}
    };
    
    // Inicializar D como matriz NxN preenchida com zeros
    vector<vector<int>> D(N, vector<int>(N, 0));
    D[0][2] = 6;  // J1 → J3
    D[1][3] = 3;  // J2 → J4

    vector<int> ILC = {0, 1, 2, 3};
    vector<int> LC = randomized_heuristic(ILC, N, S, D);

    return 0;
}