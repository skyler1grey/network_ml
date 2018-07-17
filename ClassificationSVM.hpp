/*
 * ClassificationSVM.hpp
 *
 *  Created on: Jul 16, 2018
 *      Author: root
 */

#ifndef CLASSIFICATIONSVM_HPP_
#define CLASSIFICATIONSVM_HPP_


#include "svm.h"

#include <vector>

#include <list>

#include <iostream>

#include <string.h>

#include <string>

#include <stdio.h>

#define FEATUREDIM 4


using namespace std;

class ClassificationSVM

{

public:

	ClassificationSVM();

	~ClassificationSVM();

	void train(const std::string& modelFileName);

	void predict(const std::string& featureaFileName, const std::string& modelFileName);



private:

	void setParam();

	void readTrainData(const std::string& featureFileName);

private:

	svm_parameter param;

	svm_problem prob;//all the data for train

	std::list<svm_node*> dataList;//list of features of all the samples

	std::list<double>  typeList;//list of type of all the samples

	int sampleNum;

	//bool* judgeRight;

};


void ClassificationSVM::setParam()

{

	param.svm_type = C_SVC;

	param.kernel_type = RBF;

	param.degree = 3;

	param.gamma = 0.5;

	param.coef0 = 0;

	param.nu = 0.5;

	param.cache_size = 40;

	param.C = 500;

	param.eps = 1e-3;

	param.p = 0.1;

	param.shrinking = 1;

	param.nr_weight = 0;

	param.weight = NULL;

	param.weight_label = NULL;

};


void ClassificationSVM::readTrainData(const string& featureFileName)

{

	FILE *fp = fopen(featureFileName.c_str(), "r");

	if (fp == NULL)

	{

		cout << "open feature file error!" << endl;

		return;

	}



	fseek(fp, 0L, SEEK_END);

	long end = ftell(fp);

	fseek(fp, 0L, SEEK_SET);

	long start = ftell(fp);

	//读取文件，直到文件末尾

	while (start != end)

	{

		//FEATUREDIM是自定义变量，表示特征的维度

		svm_node* features = new svm_node[FEATUREDIM + 1];//因为需要结束标记，因此申请空间时特征维度+1

		for (int k = 0; k < FEATUREDIM; k++)

		{

			double value = 0;

			fscanf(fp, "%lf", &value);

			features[k].index = k + 1;//特征标号，从1开始

			features[k].value = value;//特征值

		}

		features[FEATUREDIM].index = -1;//结束标记

		char c;

		fscanf(fp, "\n", &c);

		char name[100];

		fgets(name, 100, fp);

		name[strlen(name) - 1] = '\0';



		//negative sample type is 0

		int type = 0;

		//positive sample type is 1

		if (featureFileName == "PositiveFeatures.txt")

			type = 1;

		dataList.push_back(features);

		typeList.push_back(type);

		sampleNum++;

		start = ftell(fp);

	}

	fclose(fp);

};


void ClassificationSVM::train(const string& modelFileName)

{

	cout << "reading positivie features..." << endl;

	readTrainData("PositiveFeatures.txt");

	cout << "reading negative features..." << endl;

	readTrainData("NegativeFeatures.txt");

	cout << sampleNum << endl;

	prob.l = sampleNum;//number of training samples

	prob.x = new svm_node *[prob.l];//features of all the training samples

	prob.y = new double[prob.l];//type of all the training samples

	int index = 0;

	while (!dataList.empty())

	{

		prob.x[index] = dataList.front();

		prob.y[index] = typeList.front();

		dataList.pop_front();

		typeList.pop_front();

		index++;

	}



	cout << "start training" << endl;

	svm_model *svmModel = svm_train(&prob, &param);



	cout << "save model" << endl;

	svm_save_model(modelFileName.c_str(), svmModel);

	cout << "done!" << endl;

};


void ClassificationSVM::predict(const string& featureFileName, const string& modelFileName)

{

	std::vector<bool> judgeRight;

	svm_model *svmModel = svm_load_model(modelFileName.c_str());

	FILE *fp;

	if ((fp = fopen(featureFileName.c_str(), "rt")) == NULL)

		return;



	fseek(fp, 0L, SEEK_END);

	long end = ftell(fp);

	fseek(fp, 0L, SEEK_SET);

	long start = ftell(fp);

	while (start != end)

	{

		svm_node* input = new svm_node[FEATUREDIM + 1];

		for (int k = 0; k<FEATUREDIM; k++)

		{

			double value = 0;

			fscanf(fp, "%lf", &value);

			input[k].index = k + 1;

			input[k].value = value;

		}

		char c;

		fscanf(fp, "\n", &c);

		char name[100];

		fgets(name, 100, fp);

		name[strlen(name) - 1] = '\0';



		input[FEATUREDIM].index = -1;

		int predictValue = svm_predict(svmModel, input);

		if (featureFileName == "positive_test.txt")

		{

			if (predictValue == 0)

				judgeRight.push_back(false);

			else

				judgeRight.push_back(true);

		}

		else if (featureFileName == "negative_test.txt")

		{

			if (predictValue == 1)

				judgeRight.push_back(false);

			else

				judgeRight.push_back(true);

		}

		start = ftell(fp);

	}

	fclose(fp);



	int correctNum = 0;

	int totalNum = judgeRight.size();

	for (int i = 0; i < totalNum; i++)

	{

		if (judgeRight[i] == true)

			correctNum++;

	}

	double precent = 1.0 * correctNum / totalNum;

	cout << precent << endl;

};




#endif /* CLASSIFICATIONSVM_HPP_ */
