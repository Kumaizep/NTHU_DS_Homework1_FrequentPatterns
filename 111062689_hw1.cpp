#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iomanip>

#include <algorithm>
#include <vector>
#include <utility>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

struct FPNode
{
	short key;
	int count;
	bool picked;
	int process_count;
	FPNode* parent;
	std::unordered_map<short, FPNode*> children;
	FPNode(): key(-1), count(0), picked(false), process_count(0), parent(nullptr) {}
};

std::vector<std::pair<std::pair<int, short>, std::vector<FPNode*>>> header_table;
std::unordered_map<short, int> header_table_index;

void insert_transaction(FPNode* root, std::vector<short>::iterator beginpos, std::vector<short>::iterator endpos)
{
	root->count++;

	if (beginpos == endpos)
		return;

	short val = *beginpos;
	// std::cout << " DEBUG IT-1 " << val << std::endl;
	if (root->children.find(val) == root->children.end())
	{
		root->children[val] = new FPNode();
		root->children[val]->key = val;
		root->children[val]->parent = root;
		header_table[header_table_index[val]].second.push_back(root->children[val]);
	}
	insert_transaction(root->children[val], beginpos + 1, endpos);
	return;
}

void dfs_conut(FPNode* root, std::unordered_map<short, int>& count_sum)
{
	for (auto it: root->children)
	{
		FPNode* child = it.second;
		if (child->picked)
		{
			if (count_sum.find(child->key) == count_sum.end())
				count_sum[child->key] = child->process_count;
			else
				count_sum[child->key] += child->process_count;
			dfs_conut(child, count_sum);
		}

	}
}

void insert_transaction_partial(FPNode* root, std::vector<short>::iterator beginpos, std::vector<short>::iterator endpos, int transaction_count)
{
	root->count += transaction_count;

	if (beginpos == endpos)
		return;

	// std::cout << "  DEBUG ITP-1" << std::endl;
	short val = *beginpos;
	// std::cout << "  DEBUG ITP-2" << std::endl;
	if (root->children.find(val) == root->children.end())
	{
		root->children[val] = new FPNode();
		root->children[val]->key = val;
		root->children[val]->parent = root;
	}
	insert_transaction_partial(root->children[val], beginpos + 1, endpos, transaction_count);
	return;
}

void mine_frequent_patterns(FPNode* root, std::vector<short> curr_pattern, short suffix, 
	std::map<std::vector<short>, int>& patterns)
{
	// std::cout << "   MSG-MFP-1 " << root->key << " " << root->count << std::endl;
	if (root->key == suffix)
	{
		curr_pattern.push_back(suffix);
		if (patterns.find(curr_pattern) == patterns.end())
			patterns[curr_pattern] = root->count;
		else
			patterns[curr_pattern] += root->count;
		// std::cout << "   MSG-MFP-2 patten ";
		// for (auto it: curr_pattern)
		// 	std::cout << it << " ";
		// std::cout << ": collect " << patterns[curr_pattern] << std::endl;

	}
	else
	{
		for (auto it: root->children)
		{
			std::vector<short> pattern = curr_pattern;
			mine_frequent_patterns(it.second, pattern, suffix, patterns);
			if (root->key != -1)
			{
				pattern.push_back(root->key);
				mine_frequent_patterns(it.second, pattern, suffix, patterns);
			}
		}
	}
}		

std::map<std::vector<short>, int> mining_tree(FPNode* root, short suffix, double min_sup)
{
    // std::cout << " DEBUG MT-1 " << suffix << std::endl;
	for(FPNode* it: header_table[header_table_index[suffix]].second)
	{
		FPNode* curr = it;
		int suffix_count = curr->count;
		while (curr != root)
		{
			curr->process_count += suffix_count;
			curr->picked = true;
			curr = curr->parent;
		}
	}

	// build value sum map
    // std::cout << " DEBUG MT-2" << std::endl;
    std::unordered_map<short, int> count_sum;
    dfs_conut(root, count_sum);
    // for (auto it: count_sum)
    // 	std::cout << "  " << it.first << " " << it.second << std::endl;
    std::vector<std::pair<short, int>> sorted_count_sum;
    sorted_count_sum.reserve(count_sum.size());
	for (auto it: count_sum)
		if(it.second >= min_sup) 
			sorted_count_sum.push_back(std::pair(it.first, it.second));
	std::sort(sorted_count_sum.begin(), sorted_count_sum.end(),
		[] (const std::pair<short, int>& a, const std::pair<short, int>& b) -> bool
		{
			return a.second > b.second;
		});
	// for (auto it: sorted_count_sum)
    // 	std::cout << "  " << it.first << " " << it.second << std::endl;

    // recollect partial transactions
    // std::cout << " DEBUG MT-3" << std::endl;
    std::vector<std::pair<std::unordered_set<short>, int>> partial_transactions;
    for(FPNode* it: header_table[header_table_index[suffix]].second)
	{
    	// std::cout << "  pick leaf count " << it->count << std::endl;

		FPNode* curr = it;
		std::pair<std::unordered_set<short>, int> partial_transaction;
		partial_transaction.second = curr->count;
		curr = curr->parent;
		while (curr != root)
		{
			if (count_sum[curr->key] >= min_sup)
				partial_transaction.first.insert(curr->key);
			curr = curr->parent;
		}
		// partial_transaction.first.insert(suffix);
		partial_transactions.push_back(partial_transaction);
	}

	// rearrange partial transactions
    // std::cout << " DEBUG MT-4" << std::endl;
    std::vector<std::pair<std::vector<short>, int>> arranged_partial_transactions;
    for (auto partial_transaction: partial_transactions)
    {
    	std::pair<std::vector<short>, int> arranged_partial_transaction;
    	arranged_partial_transaction.second = partial_transaction.second;
    	for (auto key_pair: sorted_count_sum)
    		if (partial_transaction.first.count(key_pair.first) != 0)
    			arranged_partial_transaction.first.push_back(key_pair.first);
    	arranged_partial_transaction.first.push_back(suffix);
    	arranged_partial_transactions.push_back(arranged_partial_transaction);
    }

    // construct partial FP tree
    // std::cout << " DEBUG MT-5" << std::endl;
    // for (auto apts: arranged_partial_transactions)
    // {
    // 	for (auto it: apts.first)
    // 		std::cout << " " << it << ",";
    // 	std::cout << " :" << apts.second << std::endl;
    // }

    FPNode* proot = new FPNode();
    for (auto it: arranged_partial_transactions)
    	insert_transaction_partial(proot, it.first.begin(), it.first.end(), it.second);

    // mine the frequent patterns
    // std::cout << " DEBUG MT-6" << std::endl;
    std::map<std::vector<short>, int> patterns;
    std::vector<short> curr_pattern;
    mine_frequent_patterns(proot, curr_pattern, suffix, patterns);

	std::map<std::vector<short>, int> picked_patterns;
    for (auto pt: patterns)
    {
    	if (pt.second >= min_sup)
    	{
    		picked_patterns[pt.first] = pt.second;
    		// std::cout << " PICKED";
    	}
    	// for (auto it: pt.first)
    	// 	std::cout << " " << it << ",";
    	// std::cout << " :" << pt.second << std::endl;
    }

    // reset vars in FP tree
    // std::cout << " DEBUG MT-7" << std::endl;
    for(FPNode* it: header_table[header_table_index[suffix]].second)
	{
		FPNode* curr = it;
		while (curr != nullptr)
		{
			curr->process_count = 0;
			curr->picked = false;
			curr = curr->parent;
		}
	}
    // std::cout << " DEBUG MT-8" << std::endl;
    return picked_patterns;
}

int main(int argc, char const *argv[])
{
	// arg setup
	double min_sup = atof(argv[1]);
	std::ifstream fin;
	std::ofstream fout;
	fin.open(argv[2]);
    fout.open(argv[3]);

    // build transactions data base
    // std::cout << "check " << "begin 1" << std::endl; 
    std::vector<std::unordered_set<short>> transactions;
    std::string line;

    while(std::getline(fin, line))
    {
    	std::istringstream iss(line);
	    std::unordered_set<short> transaction;
	    short input;
	    char dot;
	    iss >> input;
	    transaction.insert(input);
	    while (iss >> dot >> input)
	    	transaction.insert(input);
	    transactions.push_back(transaction);
    }
    fin.close();
	min_sup *= transactions.size();
	// std::cout << "check " << "info 1" << std::endl;
    // for (auto transaction: transactions)
    // {
    // 	for(auto it: transaction)
    // 		std::cout << it << ",";
    // 	std::cout << std::endl;
    // }
    // std::cout << "check " << "end 1" << std::endl; 

    // build header table
    // std::cout << "check " << "begin 2" << std::endl; 
    for (auto transaction: transactions)
    {
    	for (auto it: transaction)
    	{
    		if (header_table_index.find(it) == header_table_index.end())
    		{
    			header_table_index[it] = header_table.size();
    			header_table.push_back(std::pair<std::pair<int, short>, std::vector<FPNode*>>(std::pair(1, it), std::vector<FPNode*>()));
    		}
    		else
    			header_table[header_table_index[it]].first.first++;
    	}
    }
    sort(header_table.begin(), header_table.end(),
    	[] (const std::pair<std::pair<int, short>, std::vector<FPNode*>> &a, 
    		const std::pair<std::pair<int, short>, std::vector<FPNode*>> &b) -> bool
    	{ 
    		return a.first.first > b.first.first; 
    	});
    int count = 0;
    for (auto it: header_table)
    	header_table_index[it.first.second] = count++;
    // std::cout << "check " << "info 2" << std::endl;
    // for (auto it: header_table)
    // 	std::cout << it.first.first << " " << it.first.second << std::endl;
    // std::cout << std::endl;
    // for (auto it: header_table_index)
    // 	std::cout << it.first << " " << it.second << std::endl;
    // std::cout << "check " << "end 2" << std::endl;

    // erase useless part of header table
    // std::cout << "check " << "begin 3" << std::endl; 
    auto htit = header_table.begin();
    while (htit != header_table.end() && (*htit).first.first >= min_sup) htit++;
    header_table.erase(htit, header_table.end());
    // std::cout << "check " << "info 3" << std::endl;
    // for (auto it: header_table)
    // 	std::cout << it.first.first << " " << it.first.second << std::endl;
    // std::cout << "check " << "end 3" << std::endl;

    // rebuild transactions data base
    // std::cout << "check " << "begin 4" << std::endl; 
    std::vector<std::vector<short>> cleared_transactions;
    cleared_transactions.reserve(transactions.size());
    for (auto transaction: transactions)
    {
    	std::vector<short> cleared_transaction;
    	for (auto it: header_table)
    	{
    		// std::cout << "\tDEBUG M-4-1 " << it.first.second << " " << transaction.count(it.first.second) << std::endl;
    		if (transaction.count(it.first.second) != 0)
    			cleared_transaction.push_back(it.first.second);
    	}
    	// std::cout << "\tDEBUG M-4-2" << std::endl;
    	cleared_transactions.push_back(cleared_transaction);
    }
    // std::cout << "check " << "info 4" << std::endl;
    // for (auto transaction: cleared_transactions)
    // {
    // 	for (auto it: transaction)
    // 		std::cout << it << ",";
    // 	std::cout << std::endl;
    // }
    // std::cout << "check " << "end 4" << std::endl; 

    // construct FP tree
    // std::cout << "check " << "begin 5" << std::endl; 
    FPNode* root = new FPNode();
    for (auto transaction: cleared_transactions)
    {
    	// std::cout << "DEBUG M5-1" << std::endl;
    	insert_transaction(root, transaction.begin(), transaction.end());
    }
    // std::cout << "check " << "end 5" << std::endl; 

    // mininf FP tree
    // std::cout << "check " << "begin 6" << std::endl; 
    std::map<std::vector<short>, int> fps;
    for (auto it: header_table)
	{
    	// std::cout << "DEBUG M6-1" << std::endl;
		std::map<std::vector<short>, int> patterns = mining_tree(root, it.first.second, min_sup);
    	// std::cout << "DEBUG M6-2" << std::endl;
		for (auto pattern: patterns)
			fps.insert(pattern);
	}
    // std::cout << "check " << "end 6" << std::endl; 

	// std output
    // std::cout << "check " << "begin 7" << std::endl;
	// for (auto it: fps)
	// {
	// 	for (auto i = it.first.begin(); i != it.first.end(); ++i)
	// 	{
	// 		std::cout << *i;
	// 		if (i != it.first.end() - 1)
	// 			std::cout << ",";
	// 		else
	// 			std::cout << ":";
	// 	}
	// 	std::cout << std::fixed << std::setprecision(4) << (double)it.second / transactions.size() << std::endl;
	// }
    // std::cout << "check " << "end 7" << std::endl;

    // file output
    for (auto it: fps)
	{
		for (auto i = it.first.begin(); i != it.first.end(); ++i)
		{
			fout << *i;
			if (i != it.first.end() - 1)
				fout << ",";
			else
				fout << ":";
		}
		fout << std::fixed << std::setprecision(4) << (double)it.second / transactions.size() << std::endl;
	}
	fout.close();


	return 0;
}