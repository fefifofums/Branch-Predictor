
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
// By: William Bogardus
// Class: COSC 3330 - Computer Architecture Spring 2017
// Explored the effectiveness of branch direction prediction of the using bimodal and global branch prediction of the Intel Pentium M with methods from "Experiment Flows and Microbenchmarks for 
// Reverse Engineering of Brannch Prediction Structures" by Vladimir Uzelac and Aleksandar Milenkovic.  
//
// Pentium M hybrid branch predictors
// This class implements a simple hybrid branch predictor based on the Pentium M branch outcome prediction units. 
// Instead of implementing the complete Pentium M branch outcome predictors, the class below implements a hybrid 
// predictor that combines a bimodal predictor and a global predictor. 

//similar to how history is updated in gshare, gets 1's in relevent bits to & them later
unsigned int relevantBits(unsigned int x, unsigned int y) {
		unsigned r = 0;
		for (unsigned i=x; i<=y; i++) r |= 1 << i;
		return r;
	}

class pm_update : public branch_update {
public:
        unsigned int index;
	unsigned int gindex;
	unsigned int gtag;
};




class pm_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH	15
#define BI_TABLE 12
	pm_update u;
	branch_info bi;
	unsigned int ghistory;
	unsigned int bhistory;

	int predictor = 1;

	unsigned char table[1<<BI_TABLE];  //bimodal table
	unsigned char gtable[1<<6][1<<9];  //global table	

	// PIR[14:0] = (PIR[12:0]<<2) xor (cbt * IP[18:4] | ibt * IP[18:10]+TA[5:0]) 	
	
	unsigned int pir = relevantBits(0,14);        //111111111111111  
	unsigned int pir_ini = relevantBits(0,12);    //1111111111111
	unsigned int pir_IP_cbt = relevantBits(4,18); //1111111111111110000
	unsigned int pir_IP_ibt = relevantBits(10,18);//1111111110000000000
	unsigned int TA_ibt = relevantBits(0,5);      //111111

	// HASH[14:0] = (IP[18:13] xor PIR[5:0]) + (IP[12:4] xor PIR[14:6])

	unsigned int hash = relevantBits(0,14);	      //111111111111111
	unsigned int hash_IP_a = relevantBits(13,18); //1111110000000000000
	unsigned int hash_pir_a = relevantBits(0,5);  //111111
	unsigned int hash_IP_b = relevantBits(4,12);  //1111111110000
	unsigned int hash_pir_b = relevantBits(6,14); //111111111000000
	
	pm_predictor (void)
	{ 
	
	}
		
	
	branch_update *predict (branch_info & b){
		bi = b;
		if (bi.br_flags & BR_CONDITIONAL){
			// HASH[14:0] = (IP[18:13] xor PIR[5:0]) + (IP[12:4] xor PIR[14:6])
			hash = ((((bi.address & hash_IP_a) >> 13) ^ (hash_pir_a & pir)) << 9) + (((hash_IP_b & bi.address) >> 4) ^ ((hash_pir_b & pir) >> 6));
			//get table indices; gindex, gtag, bimodal index
			u.gindex = hash & hash_pir_b; // HASH[5:0]
			u.gtag = (hash & hash_pir_a) >> 6; // HASH[14:6]
			u.index = bi.address & ((1<<BI_TABLE)-1); //credit to gshare code

			if (predictor == 1){
				if (table[u.index] >= 2) {
					bhistory <<= 1; 
					bhistory |= 1; // bimodal predicted history
					bhistory &= (1<<2)-1;
					u.direction_prediction(1);
				}
				else {
					bhistory <<= 1; 
					bhistory |= 0; // bimodal predicted history
					bhistory &= (1<<2)-1;
					u.direction_prediction(0);
				}
			}
			else {
				if (gtable[u.gtag][u.gindex] >= 2) {
					ghistory <<= 1;
					ghistory |= 1; // global predicted history
					ghistory &= (1<<2)-1;
					u.direction_prediction(1);
				}
				else{
					ghistory <<= 1;
					ghistory |= 0; // global predicted history
					ghistory &= (1<<2)-1;
					u.direction_prediction(0);
				}
			}
		} 
		

		u.target_prediction (0);
		return &u;
	}
		
	void update (branch_update *u, bool taken, unsigned int target) {
		// PIR[14:0] = (PIR[12:0]<<2) xor (cbt * IP[18:4] | ibt * IP[18:10]+TA[5:0]) 	
		pir = ((pir & pir_ini) << 2) ^ ((BR_CONDITIONAL*((bi.address & pir_IP_cbt) >> 5)) | (BR_INDIRECT*(((bi.address & pir_IP_ibt)>>4) + (target & TA_ibt))));	 
		
		// updates FSM at correct index (credit code to whomever wrote the given gshare update from above)
		if (bi.br_flags & BR_CONDITIONAL) {
			//global FSMs
			unsigned char *g = &gtable[((pm_update*)u)->gtag][((pm_update*)u)->gindex];
				if (taken) {
					if (*g < 3) (*g)++;
				}
				else if (*g > 0) (*g)--;
			// bimodal FSMs
			unsigned char *b = &table[((pm_update*)u)->index];
				if (taken) {
					if (*b < 3) (*b)++;
				}
				else if (*b > 0) (*b)--;
			// 1 bit predictor to choose between bimodal and global
			if ((taken != (ghistory & 1)) && (taken == (bhistory & 1))) predictor = 1;
			else if ((taken == (ghistory & 1)) && (taken != (bhistory & 1))) predictor = 0;

		}
	}

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


