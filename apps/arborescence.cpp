#include<hrlz/Arborescence.hpp>
#include<iostream>
#include<queue>
#include<algorithm>
#include<cstdint>
using namespace std;

void print_result(vector<WeightedEdge> E, std::size_t r, std::size_t n){
    vector<vector<WeightedEdge>> adjacent_children(n);
    vector<std::size_t> parent(n, 0);
    std::size_t sum = 0;
    for(const auto& e: E){
        adjacent_children[e.from].push_back(e);
        parent[e.to] = e.from;
        sum += e.weight;
    }
    cout << sum << endl;
    vector<std::size_t> distance(n, SIZE_MAX);
    queue<std::size_t> q;
    q.push(r);
    distance[r] = 0;
    //BFS type updation of distance
    while(q.size()){
      std::size_t k = q.front();
      q.pop();
      for(const auto& e: adjacent_children[k]){
        if(distance[e.to] == SIZE_MAX) {
          distance[e.to] = distance[k] + e.weight;
          q.push(e.to); 
        }
      }
    }
    for(std::size_t i = 0; i < n; ++i) {
        cout << distance[i] << " ";
    }
    cout << endl;
    for(std::size_t i = 0; i < n; ++i) {
      if(i == r)
        cout << "-1 ";
    else
        cout << parent[i] << " ";
    }
    cout << endl;
}

int main(){
  std::size_t t;
  cin >> t;
  while(t--) {
    std::size_t m, n, r;
	  cin >> n >> m >> r;
	  
    set<WeightedEdge> E;
	  vector<vector<std::size_t>> X(n,vector<std::size_t>(n,0));
    for(std::size_t i = 0; i < m; ++i) {
      std::size_t a, b, w;
	    cin >> a >> b >> w;
	    
	    if(b == r)
	        continue;
      E.insert(WeightedEdge(a,b,w));
	    X[a][b] = w;
	  }

    auto result = arborescence(E, r, n);
    print_result(result, r, n);
  }
}
