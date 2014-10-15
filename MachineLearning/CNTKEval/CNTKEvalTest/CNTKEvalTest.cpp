// CNTKEvalTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Eval.h"
#include "DataReader.h"
#include "commandArgUtil.h"
using namespace Microsoft::MSR::CNTK;

// process the command
template <typename ElemType>
void DoCommand(const ConfigParameters& configRoot)
{
    ConfigArray command = configRoot("command", "train");
    ConfigParameters config=configRoot(command[0]);
    ConfigParameters readerConfig = config("reader");
    readerConfig.Insert("traceLevel",config("traceLevel","0"));

    ConfigArray minibatchSize = config("minibatchSize", "256");
    intargvector mbSizeArr = minibatchSize;
    size_t mbSize = 20000; //mbSizeArr[0];
    size_t epochSize = config("epochSize", "0");
    if (epochSize == 0)
    {
        epochSize = requestDataSize;
    }

    ConfigParameters configFeatures = readerConfig("features");
    size_t dimFeatures = configFeatures("dim");
    ConfigParameters configLabels = readerConfig("labels");
    size_t dimLabels = configLabels("labelDim");
    ConfigParameters configSgd = config("SGD");
    std::wstring modelPath = configSgd("modelPath");

    std::map<std::wstring, Matrix<ElemType>*> inputMatrices;
    std::map<std::wstring, Matrix<ElemType>*> outputMatrices;
    std::wstring inputName = L"features";
    std::wstring outputName = L"CE.BFF.FF.P";
    Matrix<ElemType>* matrix = inputMatrices[inputName] = new Matrix<ElemType>(dimFeatures, mbSize);
    outputMatrices[outputName] = new Matrix<ElemType>(dimLabels, mbSize);

    std::map<std::wstring, std::vector<ElemType>*> input;
    std::map<std::wstring, std::vector<ElemType>*> output;
    std::vector<ElemType>* arr = input[inputName] = new std::vector<ElemType>(dimFeatures*mbSize);
    output[outputName] = new std::vector<ElemType>(dimLabels*mbSize);

    Eval<ElemType> eval(config);

    DataReader<ElemType>* dataReader = new DataReader<ElemType>(readerConfig);
    eval.LoadModel(modelPath);
    dataReader->StartMinibatchLoop(mbSize, 0, epochSize);
    while (dataReader->GetMinibatch(inputMatrices))
    {
        void* data = (void*)arr->data();
        size_t dataSize = arr->size()*sizeof(ElemType);
        void* mat = &(*matrix)(0,0);
        size_t matSize = matrix->GetNumElements()*sizeof(ElemType);
        memcpy_s(data, dataSize, mat, matSize);
        eval.Evaluate(input, output);
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    try
    {
        ConfigParameters config;
        ConfigParameters::ParseCommandLine(argc, argv, config);

        // get the command param set they want
        wstring logpath = config("stderr", L"");
        ConfigArray command = config("command", "train");


        //dump config info
        fprintf (stderr, "command: ");
        for (int i=0; i < command.size(); i++)
        {
            fprintf(stderr, "%s ", command[i].c_str());
        }

        //run commands
        std::string type = config("precision", "float");
        // accept old precision key for backward compatibility
        if (config.Exists("type"))
            type = config("type", "float");
        fprintf(stderr, "\nprecision = %s\n", type.c_str());
        if (type == "float")
            DoCommand<float>(config);
        else if (type == "double")
            DoCommand<double>(config);
        else
            throw runtime_error("invalid precision specified: " + type);
    }
    catch(std::exception &err)
    {
        fprintf(stderr, "EXCEPTION occurred: %s", err.what());
#ifdef _DEBUG
        DebugBreak();
#endif
        return -1;
    }
    catch(...)
    {
        fprintf(stderr, "Unknown ERROR occurred");
#ifdef _DEBUG
        DebugBreak();
#endif
        return -1;
    }    
    return 0;
}

