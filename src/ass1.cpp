/*
Samuel Besse
40088455
*/
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <random>

//struct Tile {
//    uint8_t top {0};
//    uint8_t right {0};
//    uint8_t left {0};
//    uint8_t bottom {0};
//    uint8_t ID {0};
//};
//
//struct Solution {
//    Tile permutation[64] {0};
//    uint8_t fitness {112};
//    //uint8_t species {0};
//};





int parent_tournament_size = 4;
int survivor_tournament_size = 3;

//highest fitness members of each species
/*Solution* elite0; 
Solution* elite1;
Solution* elite2;
Solution* elite3;
Solution* elite4;*/

Solution* elite;

int P_mutation = 50;

void evaluate_fitness(Solution* sol); //takes solution and fills in the fitness field
void mutate(Solution* sol, std::list<Tile> master_tile_list); //takes solution and performs in-place mutation
void crossover(Solution* parent_a, Solution* parent_b, Solution* child_a, Solution* child_b); //takes two parents and returns two children
void generate_solution(Solution* sol, std::list<Tile> master_list); //takes an empty solution and populates permutation based off master list of tiles
void rotate(Tile* tile, uint8_t number_rotations); //takes a tile and rotates it [0,3] number_rotations times
int kendall_tau_distance(Solution* solA, Solution* solB); //takes two individuals and returns the distance between them
Solution* host_tournament(std::list<Solution>& contestant_pool, uint8_t number_of_contestants, bool replacement); //pool to select from, number of participants in each tournament. true for replacement, false for no
bool contains(Tile* list, uint8_t length, Tile* target); //checks if Tile target is present the first length elements of list

int main() {
	std::list<Tile> master_tile_list;
	std::list<Solution> population;
	std::list<Solution> parent_pool;
	std::list<Solution> offspring_pool;
    long seed = time(NULL);
    std::default_random_engine generator (seed); //make uniform distribution with time based seed
    
    //first generate master tile list
    std::ifstream tile_file;
    tile_file.open("Ass1Input.txt");
    std::string tile_chars;
    std::string line;
    uint8_t tiles = 0;
    while(getline(tile_file, line)) {
    	for(uint8_t i = 0; i < line.size(); i++) {
    		if (line[i] == ' ') {
    			tile_chars = line.substr(i-4, 4);
    			Tile tile = Tile();
    			tile.top = tile_chars[0];
    			tile.right = tile_chars[1];
    			tile.bottom = tile_chars[2];
    			tile.left = tile_chars[3];
    			tile.ID = tiles++;
    			master_tile_list.push_back(tile);
    		}
    	}
    	tile_chars = line.substr(line.size()-4, 4); //get last element
		Tile tile = Tile();
		tile.top = tile_chars[0];
		tile.right = tile_chars[1];
		tile.bottom = tile_chars[2];
		tile.left = tile_chars[3];
		tile.ID = tiles++;
		master_tile_list.push_back(tile);
    }

    //next ask for population size and # generations
    int population_size = 500;
    int number_generations = 100;
    std::string pop_string;
    std::string gen_string;
    /*do {
        std::cout << "please input the population size: ";
        std::cin >> pop_string;
        try {
            population_size = std::stoi(pop_string);
        } catch (...) {
            std::cout << "please try again" << std::endl;
        }
    } while (population_size >= 1000 || population_size <= 100);
    do {
        std::cout << "please input the number of generations: ";
        std::cin >> gen_string;
        try {
            number_generations = std::stoi(gen_string);
        } catch (...) {
            std::cout << "please try again" << std::endl;
            continue;
        }
    } while (number_generations >= 100 || number_generations <= 1);*/

    //initialize population of size population_size
    elite = new Solution();
    for (int i = 0; i < population_size; i++) {
        Solution solution = Solution();
        generate_solution(&solution, master_tile_list);
        evaluate_fitness(&solution);
        population.push_back(solution);
    }

    //start of loop, evaluate termination conditions
    uint8_t number_of_runs = 0;
    while (elite -> fitness > 0 && number_of_runs < number_generations) {
        number_of_runs++;
        //next do parent selection
        //preliminary scheme: all parents have a random chance of being mutated and added to the offspring pool,
        //tounament selection for selecting parents for crossover, once selected use crowding. 
        //fill in rest of the population from tournament selection of mutations and non-crossed-over parents
        std::uniform_int_distribution<int> random_mutation (1,100);

        //randomly mutate population
        for(std::list<Solution>::iterator i = population.begin(); i != population.end(); i++) {
            if (random_mutation(generator) < P_mutation) {
                Solution child = Solution();
                child = *i; 
                mutate(&child, master_tile_list);
                evaluate_fitness(&child);
                offspring_pool.push_back(child);
            }
            offspring_pool.push_back(*i);
        }

        //host tournament for choosing parents for crossover
        std::list<Solution> temp_population = population;
        for(int i = 0; i < .25*population_size; i++) {//TODO: make sure that the typing of population_size makes it an int which isn't 0
            Solution* new_parent = host_tournament(temp_population, parent_tournament_size, false);
            parent_pool.push_back(*new_parent);
            if (temp_population.size() <= parent_tournament_size)
                temp_population = population;
        }
        population.clear();

        //crossover parents
        for(std::list<Solution>::iterator i = parent_pool.begin(); i != parent_pool.end(); i++) {
            Solution parent_a = *i;
            i++;
            if (i == parent_pool.end())
                break;
            Solution parent_b = *i;
            Solution *child_a, *child_b;
            crossover(&parent_a, &parent_b, child_a, child_b);
            evaluate_fitness(child_a);
            evaluate_fitness(child_b);
            if(kendall_tau_distance(&parent_a, child_a) + kendall_tau_distance(&parent_b, child_b) < kendall_tau_distance(&parent_b, child_a) + kendall_tau_distance(&parent_a, child_b)) { //then compete parent a with child a and parent b with child b
                if (parent_a.fitness > child_a -> fitness) {
                    population.push_back(parent_a);
                } else {
                    population.push_back(*child_a);
                }
                if (parent_b.fitness > child_b -> fitness) {
                    population.push_back(parent_b);
                } else {
                    population.push_back(*child_b);
                }
            } else { //compete parent a with child b and parent b with child a
                if (parent_a.fitness > child_b -> fitness) {
                    population.push_back(parent_a);
                } else {
                    population.push_back(*child_b);
                }
                if (parent_a.fitness > child_b -> fitness) {
                    population.push_back(parent_a);
                } else {
                    population.push_back(*child_b);
                }
            }
        }

        //survivor selection
        std::list<Solution> temp_offspring_pool = offspring_pool;
        while(population.size() < population_size) {//TODO: make sure that the typing of population_size makes it an int which isn't 0
            Solution* survivor = host_tournament(temp_offspring_pool, survivor_tournament_size, false);
            population.push_back(*survivor);
            if (temp_offspring_pool.size() <= survivor_tournament_size) //if not enough population yet refresh pool and finish selection
                temp_offspring_pool = offspring_pool;
        }
        std::cout << elite->fitness << std::endl;
    }
    


}

void generate_solution(Solution* sol, std::list<Tile> master_list) {//pass list by value, so original list is unchanged
    long seed = time(NULL);
    std::default_random_engine generator (seed); //make uniform distribution with time based seed
    std::uniform_int_distribution<int> random_rotation (0,3);
    for(uint8_t n = 63; n > 0; n--) {
        std::uniform_int_distribution<int> random_index (0,n);
        uint8_t index = random_index(generator);
        std::list<Tile>::iterator i = master_list.begin();
        std::advance(i, index);
        Tile tile = *i;
        master_list.erase(i);
        rotate(&tile, random_rotation(generator));
        sol -> permutation[63-n] = tile;
    }
    Tile tile = *master_list.begin();
    rotate(&tile, random_rotation(generator));
    sol -> permutation[63] = tile;
    std::uniform_int_distribution<int> random_species (0,4);
    //sol -> species = random_species(generator);
}

void evaluate_fitness(Solution* sol) { //may be worth the overhead to unroll all the loops to remove additional branching and arithmetic, likely done by compiler
    //first evaluate all horizontal edges
    uint8_t fitness = 0; 
    for (uint8_t j = 0; j < 8; j++) {
        for (uint8_t i = 0; i < 7; i++) {
            if (sol -> permutation[8*j + i].right != sol -> permutation[8*j + i + 1].left)
                fitness++;
        }
    }
    //then all vertical edges
    for (uint8_t j = 0; j < 7; j++) {
        for (uint8_t i = 0; i < 8; i++) {
            if (sol -> permutation[8*j + i].bottom != sol -> permutation[8*(j+1) + i].top)
                fitness++;
        }
    }
    sol -> fitness = fitness;
    /*switch(sol -> species) {
        case 0: 
        if (elite0 == NULL || fitness < elite0 -> fitness) 
            elite0 = sol;
        break;
        case 1: 
        if (elite1 == NULL || fitness < elite1 -> fitness) 
            elite1 = sol;
        break;
        case 2: 
        if (elite2 == NULL || fitness < elite2 -> fitness) 
            elite2 = sol;
        break;
        case 3: 
        if (elite3 == NULL || fitness < elite3 -> fitness) 
            elite3 = sol;
        break;
        case 4: 
        if (elite4 == NULL || fitness < elite4 -> fitness) 
            elite4 = sol;
        break;
    }
    sol -> fitness = fitness;*/
    if (fitness < elite -> fitness)
        elite = sol;
}

void mutate(Solution* sol, std::list<Tile> master_tile_list) {
    long seed = time(NULL);
    std::default_random_engine generator (seed); //make uniform distribution with time based seed
    std::uniform_int_distribution<int> random_mutation (0,9);
    std::uniform_int_distribution<int> random_index (0,63);
    uint8_t roll = random_mutation(generator);
    if (roll < 5) { //50% chance to perform swap mutation
        uint8_t index1 = random_index(generator);
        uint8_t index2 = random_index(generator);
        while (index2 == index1)
            index2 = random_index(generator);
        Tile temp;
        temp = sol -> permutation[index2];
        sol -> permutation[index2] = sol -> permutation[index1];
        sol -> permutation[index1] = temp;
    } else if (roll < 9) { //40% chance to perform rotate mutation
        std::uniform_int_distribution<int> random_rotation (0,3);
        rotate(&(sol -> permutation[random_index(generator)]), random_rotation(generator));
    } else {//10% chance to replace with newly generated solution
        generate_solution(sol, master_tile_list);
    }
}

void crossover(Solution* parent_a, Solution* parent_b, Solution* child_a, Solution* child_b) {
    long seed = time(NULL);
    std::default_random_engine generator (seed); //make uniform distribution with time based seed
    std::uniform_int_distribution<int> random_crossover (0,1);
    uint8_t roll = random_crossover(generator);
    if (roll == 0) { //perform a uniformly random crossover
        for (uint8_t i = 0; i < 64; i++) {
            uint8_t a_bnot = random_crossover(generator);
            if (a_bnot) {//select tile from a to go to a and b to b 
                bool tile_present_a = false;
                bool tile_present_b = false;
                for (uint8_t j = 0; j < i; j++) {
                    if (child_a -> permutation[j].ID == parent_a -> permutation[i].ID) //important to note that if the tile is in a it must not be in b
                        tile_present_a = true;
                }
                if (!tile_present_a) {
                    for (uint8_t j = 0; j < i; j++) {
                        if (child_b -> permutation[j].ID == parent_b -> permutation[i].ID) //important to note that if the tile is in a it must not be in b
                        tile_present_b = true;
                    }
                }

                if (tile_present_a || tile_present_b) {
                    child_b -> permutation[i] = parent_a -> permutation[i];
                    child_a -> permutation[i] = parent_b -> permutation[i];
                } else {
                    child_a -> permutation[i] = parent_a -> permutation[i];
                    child_b -> permutation[i] = parent_b -> permutation[i];
                }
            } else {
                bool tile_present_a = false;
                bool tile_present_b = false;
                for (uint8_t j = 0; j < i; j++) {
                    if (child_a -> permutation[j].ID == parent_b -> permutation[i].ID) //important to note that if the tile is in a it must not be in b
                        tile_present_a = true;
                }
                if (!tile_present_a) {
                    for (uint8_t j = 0; j < i; j++) {
                        if (child_b -> permutation[j].ID == parent_a -> permutation[i].ID) //important to note that if the tile is in a it must not be in b
                        tile_present_b = true;
                    }
                }

                if (tile_present_a || tile_present_b) {
                    child_a -> permutation[i] = parent_a -> permutation[i];
                    child_b -> permutation[i] = parent_b -> permutation[i];
                } else {
                    child_b -> permutation[i] = parent_a -> permutation[i];
                    child_a -> permutation[i] = parent_b -> permutation[i];
                }
            }
        }
    } else { //perform order crossover
        std::uniform_int_distribution<int> random_index (0,63);
        uint8_t index1 = random_index(generator);
        uint8_t index2 = random_index(generator);
        while (index2 == index1)
            index2 = random_index(generator);
        if (index2 > index1) { //swap so index1 < index2
            uint8_t temp = index2;
            index2 = index1;
            index1 = temp;
        }
        for (uint8_t i = index1; i < index2; i++) { //copies from index1 to index2 NONINCLUSIVE
            child_a -> permutation[i] = parent_a -> permutation[i];
            child_b -> permutation[i] = parent_b -> permutation[i]; 
        }
        uint8_t ja = 0;
        uint8_t jb = 0;
        for (uint8_t i = 0; i < 64; i++) {
            if (i >= index1 && i < index2) //skip over section already copied
                continue;
            while(contains(child_a -> permutation + index1, index2 - index1, &(parent_b -> permutation[jb]))) {//finds the next element in parent_b not present in child_a
                jb++;
            }
            child_a -> permutation[i] = parent_b -> permutation[jb];
            jb++;

            while(contains(child_b -> permutation + index1, index2 - index1, &(parent_a -> permutation[jb]))) {//finds the next element in parent_a not present in child_b
                ja++;
            }
            child_b -> permutation[i] = parent_a -> permutation[ja];
            ja++;
        }
    }
}


int kendall_tau_distance(Solution* solA, Solution* solB) { //distance between permutations with o(N^2) time cost, would be huge to find more efficient algorithm 
    int distance;
    for(uint8_t i = 0; i < 64; i++) {
        for(uint8_t j = 0; j < 64; j++) {
            if ((solA -> permutation[i].ID < solA -> permutation[j].ID && solB -> permutation[i].ID > solB -> permutation[j].ID) || (solA -> permutation[i].ID > solA -> permutation[j].ID && solB -> permutation[i].ID < solB -> permutation[j].ID))
                distance++;
        }
    }
}

void rotate(Tile* tile, uint8_t number_rotations) {
    uint8_t sides[4] = {tile->top, tile->right, tile->bottom, tile->left};
    tile->top = sides[number_rotations];
    tile->right = sides[(1 + number_rotations) % 4];
    tile->bottom = sides[(2 + number_rotations) % 4];
    tile->left = sides[(3 + number_rotations) % 4];
}

bool contains(Tile* list, uint8_t length, Tile* target) {
    for (uint8_t i = 0; i < length; i++) {
        if (list[i].ID == target -> ID)
            return true;
    }
    return false;
}

Solution* host_tournament(std::list<Solution>& contestant_pool, uint8_t number_of_contestants, bool replacement) {
	long seed = time(NULL);
	std::default_random_engine generator (seed); //make uniform distribution with time based seed
	std::uniform_int_distribution<int> random_index (0,63);
	std::list<Solution> contestants;
	for (uint8_t i = 0; i < number_of_contestants; i++) {
		std::list<Solution>::iterator s = contestant_pool.begin();
		std::advance(s, random_index(generator));
		contestants.push_back(*s);
	}
	Solution* fittest = &(*contestants.begin());
	for (std::list<Solution>::iterator it = contestants.begin(); it != contestants.end(); it++) {
		if (fittest->fitness < it->fitness)
			fittest = &(*it);
		if (!replacement)
			contestant_pool.erase(it);
	}

}


