using namespace std;


/* RANDOMIZADO ADAPTATIVO */
vector<char> Gulosos::randomized_heuristic(vector<char> LC, map<char, No *> &mapa_nos, int k, ofstream &file)
{

    map<char, int> degree;

    if (k == 0)
    {
        // calcula o grau de cada nó
        for (char no : LC)
            degree[no] = mapa_nos[no]->arestas.size();
    }
    else
    {
        // calcula o quanto cada grau pode dominar na lista de candidatos
        for (char no : LC)
        {
            int domain = 1; // considera ele mesmo
            for (Aresta *aresta : mapa_nos[no]->arestas)
            {
                if (!mapa_nos[aresta->id_no_alvo]->dominado)
                    domain++;
            }
            degree[no] = domain;
        }
    }

    // reordena os nós por grau e remonta a LC
    LC.clear();

    file << "\n=> LC: " << k << endl;
    while (!degree.empty())
    {
        auto max_it = degree.begin();
        for (auto it = degree.begin(); it != degree.end(); ++it)
        {
            if (it->second > max_it->second)
                max_it = it;
        }

        LC.push_back(max_it->first);
        file << "(" << max_it->first << " ; " << max_it->second << ") ";

        // remove do map pós iteração
        degree.erase(max_it);
    }
    file << endl;

    return LC;
}

//Atualiza dominio 
void Gulosos::updates_domain(map<char, No *> &mapa_nos, vector<char> S)
{
    for (char no : S)
    {
        mapa_nos[no]->dominado = true;
        for (Aresta *aresta : mapa_nos[no]->arestas)
            mapa_nos[aresta->id_no_alvo]->dominado = true;
    }
}

// lógica adaptativa de atualização da lista LC
vector<char> Gulosos::updates_LC(map<char, No *> &mapa_nos, vector<char> LC, vector<char> S)
{

    set<char> neighborhood = set<char>();

    // critério de atualização: removo os nós vizinhos de v da LC
    for (char no : S)
    {
        for (Aresta *aresta : mapa_nos[no]->arestas)
        {
            char neighbor = aresta->id_no_alvo;
            neighborhood.insert(neighbor);
        }
    }

    vector<char> new_LC = vector<char>();

    for (char no : LC)
    {
        // se ele for vizinho de algum nó em S, não o adiciono
        if (find(neighborhood.begin(), neighborhood.end(), no) != neighborhood.end())
            continue;

        // nao posso recolocar candidatos de S
        if (find(S.begin(), S.end(), no) != S.end())
            continue;

        new_LC.push_back(no);
    }

    return new_LC;
}

// algoritmo adaptativo guloso randomizado
vector<char> Gulosos::randomized_adaptative_greedy(Grafo *grafo, float alfa, ofstream &file)
{

    bool debug = true;
    map<char, No *> mapa_nos;
    vector<char> LC;

    // monta o mapa de nos para facilitar e a lista de candidatos
    for (No *no : grafo->lista_adj)
    {
        no->dominado = false; // reseta a dominancia para não influenciar em outras itr
        mapa_nos[no->id] = no;
        LC.push_back(no->id);
    }

    // euristica: inicialmente ordena os candidatos pelo grau do no
    // depois para garantir dominancia, pelo grau de cobertura, ambos de forma crescente
    int k = 0;
    LC = randomized_heuristic(LC, mapa_nos, k, file);
    vector<char> S = vector<char>();

    while (!LC.empty())
    {

        k++;
        int LCR = max(1, int(alfa * LC.size()));
        int randomized = rand() % LCR;

        // seleciona o no e LC <- LC - {v}
        char no = LC[randomized];
        LC.erase(LC.begin() + randomized);

        if (debug)
            file << "Escolhendo no: " << no << endl;

        S.push_back(no);

        // atualiza dominancia dos nós
        updates_domain(mapa_nos, S);

        // garanto todas as condições para independencia
        LC = updates_LC(mapa_nos, LC, S);

        // reorganiza a LC
        LC = randomized_heuristic(LC, mapa_nos, k, file);

        if (debug)
        {
            file << "[ S:  " << k << " ]" << endl;
            for (char no : S)
                file << no << " ";
            file << endl;
        }

        if (debug)
        {
            file << "[ LC:  " << k << " ]" << endl;
            for (char no : LC)
                file << no << " ";
            file << endl;
        }
    }

    return S;
}

bool Gulosos::check_validity(vector<char> S, Grafo *grafo, ofstream &file)
{

    set<char> domain = set<char>();
    set<char> dominated = set<char>();

    // para cada no de S ver se ele domina todos
    for (char v : S)
        for (No *no : grafo->lista_adj)
        {
            domain.insert(no->id); // sempre que eu buscar por um no
            if (v == no->id)
            {
                dominated.insert(no->id); // sempre que eu achar um nó e tudo que ele domina
                for (Aresta *aresta : no->arestas)
                    dominated.insert(aresta->id_no_alvo);
            }
        }

    // ver se para todos da solução é independente
    bool cond = true;
    for (auto v : S)
        for (No *no : grafo->lista_adj)
            if (v == no->id)
                for (Aresta *aresta : no->arestas)
                    if (find(S.begin(), S.end(), aresta->id_no_alvo) != S.end())
                    {
                        cond = false;
                        file << "(X) nao eh independente" << endl;
                        break;
                    }

    file << "Vertices: ";
    for (auto no : domain)
        file << no << " ";

    file << "\nDominados: ";
    for (auto no : domain)
        file << no << " ";

    file << "\nSolucao: ";
    for (auto no : S)
        file << no << " ";
    file << "\n";

    if (domain == dominated && cond)
    {
        file << "===> Solucao VALIDA! |S| = " << S.size() << "\n";
        return true;
    }
    else
        file << "===> Solucao INVALIDA\n";
    return false;
}
