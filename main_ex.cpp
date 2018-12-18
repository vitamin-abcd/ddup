//
// Created by Tom Wang on 2018/12/18.
//

#include <fstream>
#include <vector>
#include <chrono>
#include <memory>
#include <string>
#include <unistd.h>

#include <inference_engine.hpp>
#include <ext_list.hpp>
#include <format_reader_ptr.h>

#include <samples/common.hpp>
#include <samples/slog.hpp>
#include <samples/args_helper.hpp>
#include <pthread.h>

#include "classification_sample.h"

using namespace InferenceEngine;

ConsoleErrorListener error_listener;

void createPlugin(InferencePlugin &plugin, int index) {
    plugin = PluginDispatcher({FLAGS_pp, "../../../lib/intel64", ""}).getPluginByDevice(FLAGS_d);
    if (index == 0) {
        plugin.AddExtension(std::make_shared<Extensions::Cpu::CpuExtensions>());
    }
    plugin.SetConfig({{PluginConfigParams::KEY_CPU_BIND_THREAD, PluginConfigParams::YES}});
    printPluginVersion(plugin, std::cout);
}

void readNet(CNNNetReader &networkReader) {
    std::string binFileName = fileNameNoExt(FLAGS_m) + ".bin";

    networkReader.ReadNetwork(FLAGS_m);
    networkReader.ReadWeights(binFileName);
    CNNNetwork network = networkReader.getNetwork();

    slog::info << "Preparing input blobs" << slog::endl;
    InputsDataMap inputInfo = network.getInputsInfo();
    if (inputInfo.size() != 1) throw std::logic_error("Sample supports topologies only with 1 input");
    auto inputInfoItem = *inputInfo.begin();
    inputInfoItem.second->setPrecision(Precision::FP32);
    inputInfoItem.second->setLayout(Layout::NHWC);
    network.setBatchSize(8);

    slog::info << "Preparing output blobs" << slog::endl;
    OutputsDataMap outputInfo(network.getOutputsInfo());

    std::string firstOutputName;
    for (auto &item : outputInfo) {
        if (firstOutputName.empty()) {
            firstOutputName = item.first;
        }
        DataPtr outputData = item.second;
        if (!outputData) {
            throw std::logic_error("output data pointer is not valid");
        }
        outputData->setPrecision(Precision::FP32);
        outputData->setLayout(Layout::NC);
    }
}

void fillData(InferRequest &inferRequest, CNNNetReader &reader) {

    InputsDataMap inputInfo = reader.getNetwork().getInputsInfo();
    for (const auto &item : inputInfo) {
        Blob::Ptr input = inferRequest.GetBlob(item.first);

        FILE *pInputFile = fopen("/home/topn-demo/test_input.bin", "rb");
        float pInput[8 * 224 * 224 * 3];
        size_t readed_num = fread((void *) pInput, sizeof(float), 8 * 224 * 224 * 3, pInputFile);
        slog::info << "input size " << readed_num << " float" << slog::endl;

        auto data = input->buffer().as<PrecisionTrait<Precision::FP32>::value_type *>();

        for (size_t i = 0; i < (8 * 224 * 224 * 3); ++i) {
            data[i] = pInput[i];
        }
        fclose(pInputFile);
    }
}

void *run(void *p){

    InferRequest *infer_request = (InferRequest *)p;

    // --------------------------- 7. Do inference ---------------------------------------------------------
    slog::info << "Starting inference (" << FLAGS_ni << " iterations)" << slog::endl;

    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::duration<double, std::ratio<1, 1000>> ms;
    typedef std::chrono::duration<float> fsec;

    double total = 0.0;
    /** Start inference & calc performance **/
    for (int iter = 0; iter < FLAGS_ni; ++iter) {
        auto t0 = Time::now();
        infer_request->Infer();
        auto t1 = Time::now();
        fsec fs = t1 - t0;
        ms d = std::chrono::duration_cast<ms>(fs);
        total += d.count();
    }

    // -----------------------------------------------------------------------------------------------------
    std::cout << std::endl << "total inference time: " << total << std::endl;
    std::cout << "Average running time of one iteration: " << total / static_cast<double>(FLAGS_ni) << " ms"
              << std::endl;
    std::cout << std::endl << "Throughput: " << 1000 * static_cast<double>(FLAGS_ni) * 8 / total << " FPS"
              << std::endl;
    std::cout << std::endl;

    return 0;
}

bool ParseAndCheckCommandLine(int argc, char *argv[]) {
    // ---------------------------Parsing and validation of input args--------------------------------------
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    if (FLAGS_h) {
        showUsage();
        return false;
    }
    slog::info << "Parsing input parameters" << slog::endl;

    if (FLAGS_ni < 1) {
        throw std::logic_error("Parameter -ni should be greater than zero (default 1)");
    }

    if (FLAGS_i.empty()) {
        throw std::logic_error("Parameter -i is not set");
    }

    if (FLAGS_m.empty()) {
        throw std::logic_error("Parameter -m is not set");
    }

    return true;
}


const int NET_SIZE = 2;

int main(int argc, char *argv[]) {
    slog::info << "InferenceEngine: " << GetInferenceEngineVersion() << slog::endl;

    /** 参数转换/验证 */
    if (!ParseAndCheckCommandLine(argc, argv)) {
        return 0;
    }

    ExecutableNetwork executableNetwork[NET_SIZE];
    InferRequest inferRequest[NET_SIZE];
    InferencePlugin plugin[NET_SIZE];
    CNNNetReader reader[NET_SIZE];

    for (int i = 0; i < NET_SIZE; i++) {
        createPlugin(plugin[i],0);
        readNet(reader[i]);
        executableNetwork[i] = plugin[i].LoadNetwork(reader[i].getNetwork(), {});
        inferRequest[i] = executableNetwork[i].CreateInferRequest();
        fillData(inferRequest[i], reader[i]);
    }

    pthread_t callThd[NET_SIZE];
    for (int i = 0; i < NET_SIZE; i++) {
        int rc = pthread_create(&callThd[i], NULL, run, (void *) &inferRequest[NET_SIZE - i]);
        pthread_join(callThd[i], NULL);
    }

    slog::info << "Execution successful" << slog::endl;
    return 0;
}