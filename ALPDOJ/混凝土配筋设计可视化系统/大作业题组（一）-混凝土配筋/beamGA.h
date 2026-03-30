#pragma once
#include "individual.h"
#include <random>
#include "designEle.h"
#include "stdSize.h"

class beamGA
{
private:
    elements params;                                //输入的参数
	std::vector<Individual> population;	            //一代所有个体
	int mGen;							            //遗传代数
	int popSize;						            //一代中的个体数
	double crossoverRate;                           //交叉率
	double mutationRate;                            //变异率
    //生成随机数
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<double> dis01;
    double rand01() { return dis01(gen); }          //生成01之间的随机数
    double randRange(double min, double max) {
        return min + rand01() * (max - min);        //生成min和max间的随机数
    }
public:
    beamGA(elements p,int mg=200,int ps=100,double cr=0.8,double mr=0.05)
        :params(p), mGen(mg), popSize(ps), crossoverRate(cr), mutationRate(mr), gen(rd()) {
        population.resize(popSize);
    }
    double calH0(double h, double s, int n, int layer,int d);                        //计算h0
    double calSelfMoment(double b, double h, double L);                             //计算自重弯矩
    double calLoadMoment(double L, double P1, double P2,double d);                  //计算车辆弯矩
    double calPrice(double b, double h, double L, int n, double d);                 //计算造价
    void evaIndividual(Individual& ind);                                            //评估个体适应度
    void initPop();                                                                 //初始化种群
    Individual tourSelect();                                                        //锦标赛选择
    void crossover(Individual& p1, Individual& p2, Individual& c1, Individual& c2); //交叉
    void mutate(Individual& ind);                                                   //变异
    Individual run();                                                               //运行遗传算法

    double calDiversity();                                                          //计算多样性
    double getNewCR(double diversity);                                              //动态获得交叉率
    double getNewMR(double diversity);                                              //动态获得变异率
    Individual initInd();                                                           //获得一个随机个体
    void cataRebuild(std::vector<Individual>& pop);                                 //灾变机制
    elements getData();
    void setData(Individual& ind);
};

