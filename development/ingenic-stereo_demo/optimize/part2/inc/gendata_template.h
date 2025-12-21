#ifndef _GENERATEDATA_H_
#define _GENERATEDATA_H_
#include <stdio.h>
#include <string.h>
#include <string>
#include <map>
#include <sys/time.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <typeinfo>
#include <functional>
using namespace std;
class TestData;

template<typename Type>
class GenerateData{
private:
	template<typename TypeA>
	class Variable{
	public:
		TypeA *v;
		int count;
	};
	typedef map<string,Variable<Type>*> DataMap;
	typedef typename DataMap::iterator Iterator;

	DataMap *mapData;
	DataMap *mapDataDup;
	int dynamicRandom;
	void pushData(DataMap *fmap, Type *f,int count,const char *name)
	{
		Variable<Type> *v  = new Variable<Type>();
		v->v = f;
		v->count = count;
		fmap->insert(pair<string,Variable<Type>*>(name,v));
	}
	float genData()
	{
		float d;
		if(dynamicRandom == 0)
			d = drand48();
		else{
			d = rand() % 10000;
			d = d / 10000.0f;
		}
		Type x = (Type) (d * mRange + mMin);
		return x;
	}
	void genDatas(Type *d,int count)
	{
		for(int i = 0;i < count;i++){
			d[i] = genData();
		}
	}

	float mRange,mMin;
	float cal_snr_float32 (Type *pRef, Type *pTest, uint32_t buffSize)
	{
		float EnergySignal = 0.0, EnergyError = 0.0;
		uint32_t i;
		float SNR;
//		printf("-- buffSize= %d\n",buffSize);
		for (i = 0; i < buffSize; i++)
		{
			EnergySignal += pRef[i] * pRef[i];
			EnergyError += (pRef[i] - pTest[i]) * (pRef[i] - pTest[i]);
		}

		if(EnergySignal > 0.0f && EnergyError > 0.0f){
			SNR = 10 * log10 (EnergySignal / EnergyError);
		}
		else if(EnergyError <= 0.0f)
			SNR = 200.0f;
		else
			SNR = 0.0f;
		return (SNR);

	}

public:
	GenerateData()
	{
		dynamicRandom = 0;
		srand(time(NULL));
	}
	Type* randMalloc(int count,const char *name)
	{
		Type *f;
		f = (Type*)malloc(sizeof(Type) * count);
		if(mapData->find((char*)name) == mapData->end()){
			genDatas(f,count);
			pushData(mapData,f,count,name);
		}else{
			Variable<Type> *vf = mapData->find((char*)name)->second;
			memcpy(f,vf->v,vf->count * sizeof(Type));
			pushData(mapDataDup,f,count,name);
		}
		return f;
	}
	void setDynamicRand()
	{
		dynamicRandom = 1;
	}
	void SetDataRange(float range)
	{
		mRange = range;
	}
	void SetDataMin(float min)
	{
		mMin = min;
	}
	void Init()
	{
		mapData = new DataMap();
		mapDataDup = mapData;
	}
	void Init(GenerateData<Type> *d)
	{
		mapData = d->mapData;
		mapDataDup = new DataMap();
	}
	friend class TestData;

	void SnrCheckData(GenerateData<Type> *d)
	{
		Iterator it;
		for(it = mapDataDup->begin();it != mapDataDup->end();it++){
			float snr;
			Variable<Type> *a = it->second;
			Variable<Type> *b = d->mapDataDup->find(it->first)->second;
			printf("check %s ",it->first.c_str());
			snr = cal_snr_float32(a->v,b->v,a->count);
			if(snr < 45.0f){
				for(int i = 0;i < a->count / 4;i++){
					printf("\n%04d:",i * 4);
					for(int j = 0;j < 4;j++){
						printf("%f - %f=%f\t",a->v[i * 4 + j],b->v[i * 4 + j] ,a->v[i * 4 + j]-b->v[i * 4 + j]);
					}
				}
				printf("\n");
				if(a->count / 4 * 4 != a->count){
					printf("%04d:",a->count / 4 * 4);
					for(int i = a->count / 4 * 4;i < a->count;i++){
						//						printf("%f - %f\t",a->v[i],b->v[i]);
						printf("%f - %f=%f\t",a->v[i],b->v[i],a->v[i] - b->v[i]);
					}
					printf("\n");
				}
				printf("%s snr:%f\n",it->first.c_str(),snr);
				exit(0);
			}
			printf("snr = %f ok.\n",snr);
		}

	}
	void SnrCheckIntData(GenerateData<Type> *d)
	{
		Iterator it;
		for(it = mapDataDup->begin();it != mapDataDup->end();it++){
			float snr;
			Variable<Type> *a = it->second;
			Variable<Type> *b = d->mapDataDup->find(it->first)->second;
			printf("check %s ",it->first.c_str());
			snr = cal_snr_float32(a->v,b->v,a->count);
			if(snr < 120.0f){
				int field = 2*sizeof(Type);
				for(int i = 0;i < a->count / 4;i++){
					printf("\n%04d:",i * 4);
					for(int j = 0;j < 4;j++){
						printf("%*d - %*d\t",field,a->v[i * 4 + j],field,b->v[i * 4 + j]);
					}
				}
				printf("\n");
				if(a->count / 4 * 4 != a->count){
					printf("%04d:",a->count / 4 * 4);
					for(int i = a->count / 4 * 4;i < a->count;i++){
						printf("%*d - %*d\t",field,a->v[i],field,b->v[i]);
					}
					printf("\n");
				}
				printf("%s snr:%f\n",it->first.c_str(),snr);
				exit(0);
			}
			printf("snr = %f ok.\n",snr);
		}
	}

	void CheckData(GenerateData<Type> *d)
	{
		Iterator it;
		for(it = mapDataDup->begin();it != mapDataDup->end();it++){
			Variable<Type> *a = it->second;
			Variable<Type> *b = d->mapDataDup->find(it->first)->second;
			for(int j = 0;j < a->count;j++){
				if(a->v[j] != b->v[j]){
					int field = 2*sizeof(Type);
					printf("%s %d ->\n",it->first.c_str(),j);
					for(int n = 0;n < a->count;n++){

						if(a->v[n] != b->v[n])
						{
							int s = n / 4 * 4;
							printf("%04d:",s);
							for(int k = s;k < s + 4;k++){
								if(a->v[k] != b->v[k]){
									printf("[%0*d ",field,(Type)a->v[k]);
									printf("- %0*d]\t",field,(Type)b->v[k]);
								}else
								{
									printf(" %0*d ",field,(Type)a->v[k]);
									printf("- %0*d \t",field,(Type)b->v[k]);
								}
							}
							printf("\n");
							n = s + 4;
						}
						// if((n % 8) == 0)
						// 	printf("\n%04d:",n);

						// printf("%0*x ",field,(unsigned int)a->v[n]);
						// printf("- %0*x \t",field,(unsigned int)b->v[n]);
					}
					printf("\n");
					exit(0);
				}
			}
		}
	}

};
class TestData {
private:
	map<string,function<void()>> TestFuncs;
	long long diffTime;
public:
	GenerateData<int8_t>  genInt8;
	GenerateData<uint8_t> genUInt8;
	GenerateData<int16_t> genInt16;
	GenerateData<uint16_t> genUInt16;
	GenerateData<float>   genFloat;
	GenerateData<int32_t> genInt32;
	TestData(int rand = 0){
		diffTime = 0LL;
		AddTest("test",bind(&TestData::Test,this));
		if(rand){
			genInt8.setDynamicRand();
			genUInt8.setDynamicRand();
			genInt16.setDynamicRand();
			genUInt16.setDynamicRand();
			genFloat.setDynamicRand();
			genInt32.setDynamicRand();
		}
	}
	void AddTest(string name,const function<void()> &func){
		TestFuncs.insert(pair<string,function<void()>>(name,func));
	}
	virtual ~TestData(){

	}
	long long getDiffTime(){
		return diffTime;
	}
	virtual void InitData() = 0;
	virtual void Test() = 0;

	void* Malloc(int size)
	{
		void* v = malloc(size);
		memset(v,0,size);
		return v;
	}
	static long long getSystemTime()
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec * 1000000LL + tv.tv_usec;
	}
	void RunAll(int runCount){
		diffTime = 0;
		do{
			for(auto it = TestFuncs.begin();it != TestFuncs.end();it++){
				long long st;
				st = getSystemTime();
				it->second();
				diffTime += getSystemTime() - st;
			}
		}while(--runCount);
	}
	void Run(const string name,int runCount){
		int count = runCount - 1;
		long long st;
		st = getSystemTime();
		while(count--)
			TestFuncs[name]();
		diffTime = getSystemTime() - st;
	}
	void Init()
	{
		genInt8.Init();
		genUInt8.Init();
		genInt16.Init();
		genUInt16.Init();
		genFloat.Init();
		genInt32.Init();
		genInt8.SetDataRange(255.0f);
		genUInt8.SetDataRange(255.0f);
		genInt16.SetDataRange(65535.f);
		genUInt16.SetDataRange(65535.f);

		genFloat.SetDataRange(2.0f);
		genInt32.SetDataRange((float)0xffffffff);

		genInt8.SetDataMin(128.0f);
		genUInt8.SetDataMin(0.0f);
		genInt16.SetDataMin(32768.0f);
		genUInt16.SetDataMin(32768.0f);
		genFloat.SetDataMin(1.0f);
		genInt32.SetDataMin((float)0x80000000);

		InitData();
	}

	void Init(TestData *d)
	{
		genInt8.Init(&d->genInt8);
		genUInt8.Init(&d->genUInt8);
		genInt16.Init(&d->genInt16);
		genUInt16.Init(&d->genUInt16);
		genFloat.Init(&d->genFloat);
		genInt32.Init(&d->genInt32);

		genInt8.SetDataRange(255.0f);
		genUInt8.SetDataRange(255.0f);
		genInt16.SetDataRange(65535.f);
		genUInt16.SetDataRange(65535.f);
		genFloat.SetDataRange(2.0f);
		genInt32.SetDataRange((float)0xffffffff);

		genInt8.SetDataMin(128.0f);
		genUInt8.SetDataMin(0.0f);
		genInt16.SetDataMin(32768.0f);
		genFloat.SetDataMin(1.0f);
		genInt32.SetDataMin((float)0x80000000);

		InitData();
	}

	void CheckData(TestData *d){
		genFloat.SnrCheckData(&d->genFloat);
		genInt8.CheckData(&d->genInt8);
		genUInt8.CheckData(&d->genUInt8);
		genInt16.CheckData(&d->genInt16);
		genUInt16.CheckData(&d->genUInt16);
		genInt32.CheckData(&d->genInt32);
		//genInt32.SnrCheckIntData(&d->genInt32);
	}
	template<typename Type>
	void DumpIntData(Type *d,int count)
	{
		int field = 3*sizeof(Type);
		int i;
		for(i = 0;i < count / 8 * 8;i++)
		{
			if((i % 8 ) == 0)
				printf("\n%04d:",i);
			printf(" %*d ",field,d[i]);
		}
		if(count / 8 * 8 != 0){
			printf("\n%04d:",i);
			for(;i < count;i++)
				printf(" %*d ",field,d[i]);
		}
		printf("\n");
	}
};
/*!
   class DataInit :public TestData
   {
   public:
       //InputData.
   private:
       virtual void InitData()
       {
	       //generate random data.
	   }
   };
   class A : public DataInit
   {
   public:
       virtual void Test()
	   {
	       //Test code.
	   }
   };
   class B : public DataInit
   {
   public:
       virtual void Test()
       {
	       //Test code.
	   }
   };
   #define TESTCOUNT 1000

   int main(int argc, char *argv[])
   {
		A a;
		B b;
		a.Init();
		b.Init(&a);
		a.RunAll(TESTCOUNT);
		printf("cTest Time = %lld\n",a.getDiffTime());
		b.RunAll(TESTCOUNT);
		printf("cTestA Time = %lld\n",b.getDiffTime());
		if(diffB > 0)
		printf("Optimization rate = %.02f\n",(float)(a.getDiffTime() - b.getDiffTime()) / b.getDiffTime());
		a.CheckData(&b);
		printf("test Ok.\n");
   }
*/

#endif /* _GENERATEDATA_H_ */
