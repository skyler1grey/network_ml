#ifndef _CMSKETCH_H
#define _CMSKETCH_H

#include <algorithm>
#include <cstring>
#include <string.h>
#include "params.h"
#include "BOBHash.h"
#include <iostream>
#include <math.h>
#include "decode.hpp"

#include "ClassificationSVM.hpp"

using namespace std;

class CMSketch
{	
private:
	BOBHash * bobhash[MAX_HASH_NUM];
	int index[MAX_HASH_NUM];
	int *counter[MAX_HASH_NUM];
	int w, d;
	int MAX_CNT;
	int counter_index_size;
	uint64_t hash_value;

public:
	CMSketch(int _w, int _d)
	{
		counter_index_size = 20;
		w = _w;
		d = _d;
		
		for(int i = 0; i < d; i++)	
		{
			counter[i] = new int[w];
			memset(counter[i], 0, sizeof(int) * w);
		}

		MAX_CNT = (1 << COUNTER_SIZE) - 1;

		for(int i = 0; i < d; i++)
		{
			bobhash[i] = new BOBHash(i + 1000);
		}
	}
	void Insert(const char * str)
	{
		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, strlen(str))) % w;
			if(counter[i][index[i]] != MAX_CNT)
			{
				counter[i][index[i]]++;
			}

		}
	}
	int Query(const char *str)
	{
		int min_value = MAX_CNT;
		int temp;
		
		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, strlen(str))) % w;
			temp = counter[i][index[i]];
			min_value = temp < min_value ? temp : min_value;
		}
		return min_value;
	
	}
	void get_prone_item_are(map< vector<int>,int> &mlfreq,char query[10000000 + 10000000 / 5][100],int size,double threash,unordered_map<string, int> unmp)
	{
		int *con=new int[d];
		char *str;
		for(int i=0;i<size;i++)
		{
			str=query[i];
			int min_value = MAX_CNT;
			int temp;

			for(int i = 0; i < d; i++)
			{
				index[i] = (bobhash[i]->run(str, strlen(str))) % w;
				temp = counter[i][index[i]];
				con[i]=temp;
				min_value = temp < min_value ? temp : min_value;
			}
			int value=unmp[string(str)];
			double are=fabs(value-min_value)/(value*1.0);
			//printf("are:%lf\n",are);
			if(are>=threash)
			{
				vector<int> tmp;
				for(int j=0;j<d;j++)
				{
					tmp.push_back(con[j]);
				}
				sort(tmp.begin(),tmp.end());
				mlfreq[tmp]=value;
			}

		}
		return;
	}

	void get_prone_item_are_aae(map< vector<int>,int> &mlfreq,char query[10000000 + 10000000 / 5][100],int size,double threash,int threash2,unordered_map<string, int> unmp)
	{
		int *con=new int[d];
		char *str;
		for(int i=0;i<size;i++)
		{
			str=query[i];
			int min_value = MAX_CNT;
			int temp;

			for(int i = 0; i < d; i++)
			{
				index[i] = (bobhash[i]->run(str, strlen(str))) % w;
				temp = counter[i][index[i]];
				con[i]=temp;
				min_value = temp < min_value ? temp : min_value;
			}
			int value=unmp[string(str)];
			double are=fabs(value-min_value)/(value*1.0);
			int aae=fabs(value-min_value);
			//printf("are:%lf\n",are);
			if(are>=threash&&aae>=threash2)
			{
				vector<int> tmp;
				for(int j=0;j<d;j++)
				{
					tmp.push_back(con[j]);
				}
				sort(tmp.begin(),tmp.end());
				mlfreq[tmp]=value;
			}

		}
		return;
	}

    void get_all_item_with_aae_fangcha(map< vector<int>,double> &mlfreq,char query[10000000 + 10000000 / 5][100],int size,unordered_map<string, int> unmp)
    {
		vector<int> con;
		char *str;
		for(int i=0;i<size;i++)
		{
			str=query[i];
			int min_value1 = MAX_CNT;
			int min_value2 = MAX_CNT;
			int temp;

			for(int i = 0; i < d; i++)
			{
				index[i] = (bobhash[i]->run(str, strlen(str))) % w;
				temp = counter[i][index[i]];
				con.push_back(temp);
				if(temp<min_value1)
				{
					min_value2=min_value1;
					min_value1=temp;
				}
				else if(temp<min_value2)
				{
					min_value2=temp;
				}
			}
			int aae=min_value2-min_value1;
			double fangc=fangcha(con);
			int value=unmp[string(str)];
			sort(con.begin(),con.end());
			con.push_back(value);
			con.push_back(aae);
			mlfreq[con]=fangc;
			con.erase(con.begin(),con.end());

		}
		return;
    }

	void get_prone_item(map< vector<int>,int> &mlfreq,char query[10000000 + 10000000 / 5][100],int size,int threash,unordered_map<string, int> unmp)
	{
		int *con=new int[d];
		char *str;
		for(int i=0;i<size;i++)
		{
			str=query[i];
			int min_value1 = MAX_CNT;
			int min_value2 = MAX_CNT;
			int temp;

			for(int i = 0; i < d; i++)
			{
				index[i] = (bobhash[i]->run(str, strlen(str))) % w;
				temp = counter[i][index[i]];
				con[i]=temp;
				if(temp<min_value1)
				{
					min_value2=min_value1;
					min_value1=temp;
				}
				else if(temp<min_value2)
				{
					min_value2=temp;
				}
			}
			if(min_value2-min_value1>threash&&min_value2>=2*min_value1)
			{
				vector<int> tmp;
				for(int j=0;j<d;j++)
				{
					tmp.push_back(con[j]);
				}
				int value=unmp[string(str)];
				sort(tmp.begin(),tmp.end());
				mlfreq[tmp]=value;
			}

		}
		return;
	}

	vector<int> print_item(char * str,unordered_map<string, int> unmp)
	{
		vector<int> ttt;
		int min_value = MAX_CNT;
		int temp;

		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, strlen(str))) % w;
			temp = counter[i][index[i]];
			ttt.push_back(temp);
		}
		ttt.push_back(unmp[string(str)]);
		return ttt;
	}


	void Delete(const char * str)
	{
		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, strlen(str))) % w;
			counter[i][index[i]] --;
		}
	}
	~CMSketch()
	{
		for(int i = 0; i < d; i++)	
		{
			delete []counter[i];
		}


		for(int i = 0; i < d; i++)
		{
			delete bobhash[i];
		}
	}
};
#endif//_CMSKETCH_H
