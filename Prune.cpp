#include "Prune.h"

void PruneKOSR::main()
{
    RouteTable RT;
    HashPool HP;
    current_k = 0;
    srand(50);
    for (int q_order = 0; q_order < ArgumentManager::numQueries; q_order ++)
    {
        // resolve the query
        Query query = QueryGenerator::query_set.at(q_order);
        Node s_node = DataLoader::nodes.at(query.sourceID);
        Node t_node = DataLoader::nodes.at(query.destinationID);
        vector<int> cate_seq = query.cate_sequence;

        // initialize the route table
        RT.table_init(s_node.nodeID);
        // cout << "the size of the table is" << RT.table.size() << endl; 

        // In to the main function
        while (current_k<ArgumentManager::k)
        {
            // *** take out the route ***
            auto table_iter = RT.table.rbegin();
            Route *exam_route_p = &table_iter->back(); // To directly change the value: iter->front().route_len += 50;
            Route exam_route(*exam_route_p);
            cout << "exam size" << exam_route.route_len << endl;

            // *** prepare for the next step -- adding a new step ***
            RT.table.push_back(RT.table.back());
            auto new_table_iter = RT.table.rbegin();
            // delete the original route from the route table
            new_table_iter->pop_back();

            // *** examine the route ***
            // check if the route reached the target node
            if(exam_route.route_len == ArgumentManager::numCate+2)
            {
                // ** reconsider the dominated route **
                for (int i=1; i<ArgumentManager::numCate+2;i++)
                {
                    int rec_node_ID = exam_route.candidate.at(i);
                    int rec_len = i+1;
                    vector<Route> *v_doming = &HP.Hash_list[rec_node_ID].dominating_table;
                    vector<Route> *v_dominated = &HP.Hash_list[rec_node_ID].dominated_table;
                    for (int len_search = 0; len_search<v_doming->size(); len_search++)
                    {
                        if (v_doming->at(len_search).route_len == rec_len)
                        {
                            vector<int> wait_candidate = (v_doming->begin()+len_search)->candidate;
                            vector<int> target_route = {exam_route.candidate.begin(), exam_route.candidate.begin()+i};
                            if (target_route == wait_candidate)
                            {
                                // extract the min cost route from dominated
                                Route rec_route = HP.extract_min(v_dominated);
                                // insert the reconsider root into route table
                                new_table_iter->push_back(rec_route);
                                // erase the route in dominating
                                v_doming->erase(v_doming->begin()+len_search);
                            }
                        }
                        // v_doming->erase(v_doming->begin()+len_search);
                    }
                }
                current_k ++;        
            }
            else // not reached yet, do extend and replace
            {
                // ** take out the final and last second node **
                Node vq = DataLoader::nodes.at(exam_route.candidate.back());
                Node vl = DataLoader::nodes.at(exam_route.candidate.rbegin()[1]);
                
                // ** chekc domination **
                bool dominating = HP.check_domination(exam_route, vq.nodeID);
                // if dominating
                if (dominating)
                {
                    // add the examined route to the dominating hash table
                    cout << "find a dominating route" << endl;
                    HP.add_to_dominating(exam_route, vq.nodeID);

                    // extend the route from the final node
                    Route extended_route = exam_route;
                    int neighborID = RT.FNN(vq.nodeID, 1);
                    extended_route = RT.extend_route(extended_route, vq.nodeID, neighborID);

                    // ** add the extended route to the route table **
                    new_table_iter->push_back(extended_route);
                }
                // else if dominated
                else
                {
                    // add the examined route to the dominated hash table
                    cout << "find a dominated route" << endl;
                    HP.add_to_dominated(exam_route, vq.nodeID);
                }
                
                // do replace with the second node
                Route replaced_route = exam_route;
                int neighborID = RT.FNN(vl.nodeID, replaced_route.knn);
                replaced_route = RT.replace_route(replaced_route, vl.nodeID, neighborID);

                // ** add the replaced route to the route table **
                new_table_iter->push_back(replaced_route);
            }
        
            // *** sort the route by cost from large to small ***
            sort( new_table_iter->begin( ), new_table_iter->end( ), [ ]( const Route& lhs, const Route& rhs )
            {
            return lhs.cost > rhs.cost;
            });


            // max_element( new_table_iter->begin(), new_table_iter->end(), []( const Route &lhs, const Route &rhs )
            // {
            //     return lhs.route_len < rhs.route_len;
            // } ); 

            // (optional) print out 
            RT.print_last_step();
        }
    }
    
}

bool HashPool::check_domination(Route exam_route, int vq_ID)
{
    // Case 1: the hash table of vertex VQ has never been generated
    if (Hash_list.count(vq_ID)==0 ) 
    {
        // Generate the hash table
        Hash_table hash;
        Hash_list[vq_ID] = hash;
        return true;
    }
    // Case 2: vertex hash exist and already contain this route_len
    vector<Route> *domi_table = &Hash_list[vq_ID].dominating_table;
    for (auto iter = domi_table->rbegin() ; iter != domi_table->rend(); --iter)
    {
        if (iter->route_len == exam_route.route_len)
        return false;
    }
    // Case 3: hash of VQ exists checked
    //         no such route_len chekced
    return true;      
}

void HashPool::add_to_dominating(Route exam_route, int vq_ID)
{
    Route hash_route(vq_ID, exam_route.candidate);
    Hash_list[vq_ID].dominating_table.push_back(hash_route);
}

void HashPool::add_to_dominated(Route exam_route, int vq_ID)
{
    Route hash_route(vq_ID, exam_route.candidate, exam_route.cost);
    Hash_list[vq_ID].dominated_table.push_back(hash_route);
}

Route HashPool::extract_min(vector<Route> *route_table)
{
    Route min_route;
    return min_route;
}

void RouteTable::table_init(int start_id)
{
    table.clear();
    // insert the first route
    Route init_route;
    init_route.candidate.push_back(start_id);
    vector<Route> init_route_vec;
    init_route_vec.push_back(init_route);
    table.push_back(init_route_vec);

    print_last_step();

    // insert the second route
    Route second_route = init_route;
    int kth = 1;
    int second_id = FNN (start_id, kth);
    second_route = this->extend_route(second_route, start_id, second_id);
    vector<Route> second_route_vec;
    second_route_vec.push_back(second_route);
    table.push_back(second_route_vec);

    print_last_step();
}

int RouteTable::FNN(int source_id, int kth)
{
    return rand()%100;
}

Route RouteTable::extend_route(Route extended_route, int vqID, int neigborID)
{
    extended_route.candidate.push_back(neigborID);
    extended_route.route_len ++;
    extended_route.knn = 1;
    extended_route.cost += rand()%5;
    return extended_route;
}

Route RouteTable::replace_route(Route replaced_route, int vlID, int neigborID)
{
    replaced_route.candidate.back()=neigborID;
    replaced_route.knn ++;
    replaced_route.cost += rand()%10;
    return replaced_route;
}

void RouteTable::print_last_step()
{
    // print out the current step
    cout << "current step:" << table.size() << endl;
    vector<Route> current_step_v = table.back();
    for (int n_route = 0; n_route<current_step_v.size(); n_route++)
    {
        cout << "[ " << current_step_v.at(n_route).route_len << ": <";
        vector<int> current_vector = current_step_v.at(n_route).candidate;
        for (int i = 0; i<current_vector.size(); i++)
            cout << current_vector.at(i) << ",";
        cout << "> (" << current_step_v.at(n_route).cost << ")]  |  " ;
    }
    cout << endl;
}