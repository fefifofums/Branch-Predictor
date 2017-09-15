

// my_predictor.h
// This file contains a sample gshare_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.
#include <stdio.h>
#include <iostream>
using namespace std;
class gshare_update : public branch_update {
public:
	unsigned int index;
};

class gshare_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH	15
#define TABLE_BITS	15
	gshare_update u;
	branch_info bi;
	unsigned int history;
	unsigned char tab[1<<TABLE_BITS];

	gshare_predictor (void) : history(0) //initialize history to 0
	{ 
		memset (tab, 0, sizeof (tab)); // sets memblock starting at tab equal to 0
	}

	branch_update *predict (branch_info & b) {
		bi = b;
		if (b.br_flags & BR_CONDITIONAL) {
			
			u.index = (history << (TABLE_BITS - HISTORY_LENGTH)) ^ (b.address & ((1<<TABLE_BITS)-1));
			u.direction_prediction (tab[u.index] >> 1);
		} else {
			u.direction_prediction (true);
		}
		u.target_prediction (0);
		return &u;
	}

	void update (branch_update *u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			unsigned char *c = &tab[((gshare_update*)u)->index];
			if (taken) {
				if (*c < 3) (*c)++;
			} else {
				if (*c > 0) (*c)--;
			}
			history <<= 1;
			history |= taken;
			history &= (1<<HISTORY_LENGTH)-1;
		}
	}
};

//
// Pentium M hybrid branch predictors
// This class implements a simple hybrid branch predictor based on the Pentium M branch outcome prediction units. 
// Instead of implementing the complete Pentium M branch outcome predictors, the class below implements a hybrid 
// predictor that combines a bimodal predictor and a global predictor. 
class pm_update : public branch_update {
public:
        unsigned int index;
};

class pm_predictor : public branch_predictor {
public:
#define BI_TABLE 15
	pm_update u;
	branch_info bi;
	unsigned int table[1<<BI_TABLE];

	pm_predictor (void){}
	
	branch_update *predict (branch_info & b){
		bi = b;
		if (bi.br_flags && BR_CONDITIONAL){;
			u.index = bi.address ^ ((1<<BI_TABLE)-1);
			if (table[u.index] >= 2) u.direction_prediction(1);
			else u.direction_prediction(0);
		}

		u.target_prediction (0);
		return &u;
	}
		
	void update (branch_update *u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			unsigned int *c = &table[((pm_update*)u)->index];
				if (taken) {
					if (*c < 3) (*c)++;
				}
				else if (*c > 0) (*c)--;
		}
	}


	/*	
	void update (branch_update *u, bool taken, unsigned int target){
		if (bi.br_flags && BR_CONDITIONAL){
			bi_index = ((pm_update*)u)->address & (1<<(BI_TABLE))-1;
			if (taken) {if (table[bi_index] <= 2) table[bi_index]++;
			}
			else if (table[bi_index] >= 1) table[bi_index]--;
		}
	}
*/

/*
#define HISTORY_LENGTH	15
#define TABLE_BITS	15
	pm_update u;
	branch_info bi;
	unsigned int history;
	unsigned char tab[1<<TABLE_BITS];
	int c = 0;
	std::cout << history << endl;

	pm_predictor (void) : history(0) { 
		memset (tab, 0, sizeof (tab));
	}

	branch_update *predict (branch_info & b) {
		bi = b;
		if (b.br_flags & BR_CONDITIONAL) {
			
			u.index = (history << (TABLE_BITS - HISTORY_LENGTH)) ^ (b.address & ((1<<TABLE_BITS)-1));
			u.direction_prediction (tab[u.index] >> 1);
		} else {
			u.direction_prediction (true);
		}
		u.target_prediction (0);
		return &u;
	}

	void update (branch_update *u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			unsigned char *c = &tab[((pm_update*)u)->index];
			if (taken) {
				if (*c < 3) (*c)++;
			} else {
				if (*c > 0) (*c)--;
			}
			history <<= 1;
			history |= taken;
			history &= (1<<HISTORY_LENGTH)-1;
		
		}
	
	c++;
	if (c == 1000){
		cout << TABLE_BITS << endl;
		c = 0;
	}

	}
*/

};

//
// Complete Pentium M branch predictors for extra credit
// This class implements the complete Pentium M branch prediction units. 
// It implements both branch target prediction and branch outcome predicton. 
class cpm_update : public branch_update {
public:
        unsigned int index;
	
};

class cpm_predictor : public branch_predictor {
public:
        cpm_update u;

        cpm_predictor (void) {
        }

        branch_update *predict (branch_info & b) {
            u.direction_prediction (true);
            u.target_prediction (0);
            return &u;
        }

        void update (branch_update *u, bool taken, unsigned int target) {
        }

};


