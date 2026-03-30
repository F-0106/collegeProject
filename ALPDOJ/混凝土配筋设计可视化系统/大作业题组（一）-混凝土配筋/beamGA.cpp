#include "beamGA.h"
#include <iostream>

double beamGA::calH0(double h, double s, int n, int layer, int d)
{
	if (n <= 0 || layer <= 0)return h - c;
	std::vector<int> num;
	double sum = 0;
	for (int i = 0; i < layer; ++i) {
		int x = ceil(n / (double)layer);
		double h1 = c + i * s + i * d + d / 2.0;
		if (i != layer - 1)num.push_back(x);
		else num.push_back(n - (layer - 1) * x);
		sum += h1 * num[i];
	}
	double h0 = h - sum / n;
	return h0;
}

double beamGA::calSelfMoment(double b, double h, double L)
{
	return gamma * b * h * L * L / 8.0;
}

double beamGA::calLoadMoment(double L, double P1, double P2, double d)
{
	double maxMoment = 0, M1, M2;
	int step = 100;
	for (int i = 0; i <= L; i += step) {
		double x = 1.0 * i;
		if (x + d <= L)
			M1 = x * (L - x) * P1 / L + (x + d) * (L - x - d) * P2 / L;
		else
			M1 = x * (L - x) * P1 / L;
		if (x - d >= 0)
			M2 = x * (L - x) * P2 / L + (x - d) * (L - x + d) * P1 / L;
		else
			M2 = x * (L - x) * P2 / L;
		double curMoment = std::max(M1, M2);
		maxMoment = std::max(curMoment, maxMoment);
	}
	return maxMoment;
}

double beamGA::calPrice(double b, double h, double L, int n, double d)
{
	double price = 600 * b * h * L * 1e-9 + 500 * n + 6868.75 * (1 + (d - 14) * 0.025) * n * d * d * L * M_PI * 1e-9;
	return price;
}

void beamGA::evaIndividual(Individual& ind)
{
	double L = params.L, P = params.P, d = params.d;
	ind.b = stdWidth(ind.bRaw);		//标准化b和h
	ind.h = stdHeight(ind.hRaw);
	double b = ind.b, h = ind.h;
	double M = calLoadMoment(L, P / 2, P / 2, d) + calSelfMoment(b, h, L);

	double hpre = -100, hcur = h - c;
	int itm = 10, it = 0;
	double ksi = 0, a = 0;
	double mksi = ksib * (1 - 0.5 * ksib);
	while (fabs(hpre - hcur) > 1.0 && it++ < itm) {
		hpre = hcur;
		a = M / (a1 * fc * b * hcur * hcur);
		if (a1 * fc * b * hcur * hcur < 1e-6)break;
		if (a > mksi)a = mksi;
		ksi = 1 - sqrt(1 - 2 * a);
		ind.As = ksi * a1 * fc * b * hcur / fy;
		ind.rebar = calLayout(b, ind.As, h);
		hcur = calH0(h, ind.rebar.space, ind.rebar.n, ind.rebar.layer, ind.rebar.d);
	}
	double h0 = hcur;
	//检验受压区高度
	a = M / (a1 * fc * b * h0 * h0);
	if (a > mksi) {
		ind.feasible = false;
		ind.value = 1e18;
		ind.rebar.fitable = false;
		return;

	}
	//使配筋率大于等于最小配筋率
	ksi = 1 - sqrt(1 - 2 * a);
	ind.As = ksi * a1 * fc * b * h0 / fy;
	if (ind.As < pmin * b * h)ind.As = pmin * b * h;
	//检验承载力
	ind.rebar = calLayout(b, ind.As, h);
	if (!ind.rebar.fitable) {			//检验钢筋是否放得下
		ind.feasible = false;
		ind.value = 1e18;
		return;
	}
	ind.As = ind.rebar.n * ind.rebar.d * ind.rebar.d * M_PI / 4.0;//计算实际上的配筋面积
	ksi = fy * ind.As / (a1 * fc * b * h0);//计算实际的柯西值
	double Mu = a1 * fc * b * h0 * h0 * ksi * (1 - 0.5 * ksi);
	if (Mu <= M) {
		ind.feasible = false;
		ind.value = 1e18;
		ind.rebar.fitable = false;
		return;
	}
	ind.Mu = Mu, ind.Mmax = M;
	ind.feasible = true;
	ind.rebar.fitable = true;
	//计算造价
	ind.value = calPrice(b, h, L, ind.rebar.n, ind.rebar.d);
}

void beamGA::initPop()
{
	double minH = params.L / hmin;
	double maxH = params.L / hmax;
	double minB = minH / bmin;
	double maxB = maxH / bmax;
	for (int i = 0; i < popSize; ++i) {	
		double bStep = (maxB - minB) / (popSize - 1);
		double hStep = (maxH - minH) / (popSize - 1);
		population[i].bRaw = minB + i * bStep + randRange(-bStep / 2, bStep / 2);
		population[i].hRaw = minH + i * hStep + randRange(-hStep / 2, hStep / 2);
		evaIndividual(population[i]);
	}
}

Individual beamGA::tourSelect()
{
	std::vector<int>candis;
	while (candis.size() < 5)candis.push_back(rand() % population.size());
	Individual res = population[candis[0]];
	for (int idx : candis) {
		if (population[idx].feasible && !res.feasible)
			res = population[idx];
		else if (population[idx].feasible && res.feasible) {
			if (population[idx].value < res.value)
				res = population[idx];
		}
	}
	return res;
}

void beamGA::crossover(Individual& p1, Individual& p2, Individual& c1, Individual& c2)
{
	double cR = getNewCR(calDiversity());
	if (rand01() < cR) {
		double x = randRange(0.3, 0.7);
		c1.bRaw = x * p1.bRaw + (1 - x) * p2.bRaw + randRange(-50, 50);
		c1.hRaw = x * p1.hRaw + (1 - x) * p2.hRaw + randRange(-100, 100);
		c2.bRaw = (1 - x) * p1.bRaw + x * p2.bRaw + randRange(-50, 50);
		c2.hRaw = (1 - x) * p1.hRaw + x * p2.hRaw + randRange(-100, 100);
	}
	else {
		c1 = p1;
		c2 = p2;
	}
	evaIndividual(c1);
	evaIndividual(c2);
}

void beamGA::mutate(Individual& ind)
{
	double minH = params.L / hmin;
	double maxH = params.L / hmax;
	double minB = minH / bmin;
	double maxB = maxH / bmax;
	double mR = getNewMR(calDiversity());
	if (rand01() < mR) {
		if (rand01() < 0.3) {
			ind.bRaw = randRange(minB, maxB);
		}
		else {
			ind.bRaw += randRange(-100, 100);
			ind.bRaw = std::max(minB, std::min(ind.bRaw, maxB));
		}
		evaIndividual(ind);
	}
	if (rand01() < mutationRate) {
		if (rand01() < 0.3) {
			ind.hRaw = randRange(minH, maxH);
		}
		else {
			ind.hRaw += randRange(-200, 200);
			ind.hRaw = std::max(minH, std::min(ind.hRaw, maxH));
		}
		evaIndividual(ind);
	}
}

Individual beamGA::run()
{
	initPop();
	Individual best;
	best.value = 1e18;
	for (int i = 0; i < mGen; ++i) {
		Individual curBest;//精英遗传，当代最好的个体不参与交叉变异
		curBest.value = 1e18;
		std::vector<Individual>nPopulation;//当前代遗传结果
		for (auto& ind : population) {
			if (ind.feasible && ind.value < curBest.value)curBest = ind;
		}
		if (curBest.value < best.value)best = curBest;
		nPopulation.push_back(best);
		double diversity = calDiversity();//获取多样性
		if (diversity < 0.05)cataRebuild(population);//多样性过低则触发灾变机制
		while ((int)nPopulation.size() < popSize) {
			Individual p1 = tourSelect();
			Individual p2 = tourSelect();
			Individual c1, c2;
			crossover(p1, p2, c1, c2);//交叉
			if (i > 0) {
				mutate(c1);//变异
				mutate(c2);
			}
			if ((int)nPopulation.size() < popSize)nPopulation.push_back(c1);
			if ((int)nPopulation.size() < popSize)nPopulation.push_back(c2);
		}
		population = nPopulation;//更新种群 
	}
	setData(best);
	std::cout << best.value;
	return best;
}

double beamGA::calDiversity()
{
	std::vector<double> vals;
	for (auto& ind : population) {
		if (ind.feasible)vals.push_back(ind.h + ind.b);
	}
	double mean = 0.0, std = 0.0;// 均值和标准差
	if (vals.size() < 2)return 0.0;
	for (double v : vals) {
		mean += v;
	}
	mean /= vals.size();
	for (double v : vals) {
		std += (v - mean) * (v - mean);
	}
	std = sqrt(std / (vals.size() - 1));
	return mean < 1e-6 ? 0.0 : std / mean;
}

double beamGA::getNewCR(double diversity)
{
	double minc = 0.7, maxc = 0.95;
	if (diversity < 0.05)return maxc;
	return maxc - (diversity / 0.2) * (maxc - minc);
}

double beamGA::getNewMR(double diversity)
{
	double minm = 0.05, maxm = 0.2;
	if (diversity < 0.05)return maxm;
	return maxm - (diversity / 0.2) * (maxm - minm);
}

Individual beamGA::initInd()
{
	Individual Ind;
	double minH = params.L / hmin;
	double maxH = params.L / hmax;
	double minB = minH / bmin;
	double maxB = maxH / bmax;
	Ind.bRaw = randRange(minB, maxB);
	Ind.hRaw = randRange(minH, maxH);
	evaIndividual(Ind);
	return Ind;
}

void beamGA::cataRebuild(std::vector<Individual>& pop)
{
	std::vector<Individual> feasibleInd;
	for (auto& ind : pop) {
		if (ind.feasible)feasibleInd.push_back(ind);
	}
	sort(feasibleInd.begin(), feasibleInd.end(), [](const Individual& a, const Individual& b) {return a.value < b.value; });
	int nNum = pop.size() / 2;
	std::vector<Individual> nPop;
	for (int i = 0; i < std::min(nNum, (int)feasibleInd.size()); ++i) {
		nPop.push_back(feasibleInd[i]);
	}
	while (nPop.size() < pop.size()) {
		nPop.push_back(initInd());
	}
	pop = nPop;
}

elements beamGA::getData()
{
	return params;
}

void beamGA::setData(Individual& ind)
{
	params.b = ind.b;
	params.h = ind.h;
	params.dia = ind.rebar.d;
	params.space = ind.rebar.space;
	params.n = ind.rebar.n;
	params.layer = ind.rebar.layer;
	params.Mmax = ind.Mmax;
	params.Mu = ind.Mu;
	params.value = ind.value;
	params.hasData = true;
}
