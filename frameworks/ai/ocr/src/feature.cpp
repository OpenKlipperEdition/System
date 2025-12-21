#include <iostream>
#include <fstream>
#include <chrono>
#include <iterator>
#include <cstdlib>
#include <iomanip>
#include "threadpool.h"
#include "sys/time.h"

#include "layers/reshape.h"
#include "layers/permute.h"
#include "layers/maxpool2d_permute.h"
#include "layers/conv2d_relu_fixpoint_permute.h"

#include "layers/bidirectionalLSTM_fixpoint_msa.h"
#include "layers/bidirectionalLSTM_f16_msa.h"

#include "layers/linear_f16_msa.h"
#include "layers/linear_fixpoint_msa.h"

#include "layers/conv2d_fixpoint_permute_msa.h"
#include "layers/conv2d_norm_fixpoint_permute_msa.h"

#include "layers/groupconv2d_relu_permute_msa.h"
#include "layers/groupconv2d_norm_relu_permute_msa.h"

struct program_options {
  const char *lstm, *pca, *output, *image;
  const char *verify;
  bool binary, verbose, isjpg, normalized;
  std::size_t threads_num, batch_size;
  std::vector<const char *> files;
};

program_options parse_args(int argc, const char *argv[]);
void print_options(const program_options &options);
std::shared_ptr<tnn::layer> load_lstm(const char *filename);
std::shared_ptr<tnn::layer> load_pca(const char *filename);
template <typename Iterator>
tnn::tensor<> load_sample(Iterator first, Iterator last, tnn::thread_pool &threads);
tnn::tensor<> load_raw_features(const char *filename);
void save_result(std::ostream &out, const tnn::tensor<> &result, bool binary);
tnn::tensor_uint8 load_image(const char *filename);
template <typename Rep, typename Period>
std::ostream &operator << (std::ostream &out, const std::chrono::duration<Rep, Period> &duration);

std::shared_ptr<tnn::layer> load_lstm(const char *filename) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (!in) {
    std::cerr << "feature: failed to open Lstm data file \"" << filename << "\"" << std::endl;
    std::exit(1);
  }
  // (conv1): Conv2d(24, 24, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), groups=24)
  in.seekg(0);
  std::shared_ptr<tnn::layer > lstm = std::make_shared<tnn::layers<> >(std::initializer_list<std::shared_ptr<tnn::layer > >({ //
            // 0
            std::make_shared<tnn::conv2d_relu_fixpoint_permute>(1, 24, 5, 2, 2),
            // 1
            std::make_shared<tnn::groupconv2d_relu_permute_msa>(24, 24, 3, 24, 1, 1),
            std::make_shared<tnn::conv2d_fixpoint_permute_msa>(24, 128, 1, 1, 0),
            std::make_shared<tnn::maxpool2d_permute>(std::initializer_list<size_t>({2, 2}), std::initializer_list<size_t>({2, 2}), std::initializer_list<size_t>({0, 0})),

            // 2
            std::make_shared<tnn::groupconv2d_norm_relu_permute_msa>(128, 128, 3, 128, 1, 1),
            std::make_shared<tnn::conv2d_norm_fixpoint_permute_msa>(128, 256, 1, 1, 0),

            // 3
            std::make_shared<tnn::groupconv2d_relu_permute_msa>(256, 256, 3, 256, 1, 1),
            std::make_shared<tnn::conv2d_fixpoint_permute_msa>(256, 256, 1, 1, 0),
            std::make_shared<tnn::maxpool2d_permute>(std::initializer_list<size_t>({2, 2}), std::initializer_list<size_t>({2, 1}), std::initializer_list<size_t>({0, 1})),

            // 4
            std::make_shared<tnn::groupconv2d_norm_relu_permute_msa>(256, 256, 3, 256, 1, 1),
            std::make_shared<tnn::conv2d_norm_fixpoint_permute_msa>(256, 512, 1, 1, 0),

            // 5
            std::make_shared<tnn::groupconv2d_relu_permute_msa>(512, 512, 3, 512, 1, 1),
            std::make_shared<tnn::conv2d_fixpoint_permute_msa>(512, 512, 1, 1, 0),
            std::make_shared<tnn::maxpool2d_permute>(std::initializer_list<size_t>({2, 2}), std::initializer_list<size_t>({2, 1}), std::initializer_list<size_t>({0, 1})),

            // // 6
            std::make_shared<tnn::groupconv2d_norm_relu_permute_msa>(512, 512, 2, 512, 1, 0),
            std::make_shared<tnn::conv2d_norm_fixpoint_permute_msa>(512, 512, 1, 1, 0),
            std::make_shared<tnn::permute>(std::initializer_list<size_t>({0,2,1,3})),
            std::make_shared<tnn::reshape>(std::initializer_list<size_t>({512})),

            // rnn
            std::make_shared<tnn::BidirectionalLSTM_f16_msa>(512, 128),
            std::make_shared<tnn::Linear_fixpoint_msa>(128, 256),
            std::make_shared<tnn::BidirectionalLSTM_f16_msa>(256, 64),
            std::make_shared<tnn::Linear_fixpoint_msa>(64, 5530),
            }));

  lstm->load(in);

  in.close();
  return lstm;
}

#include <iostream>
#include <stdio.h>
using namespace std;
void save_binary_file(const char * filename, tnn::tensor<> &data)
{
  std::ofstream out;
  out.open(filename, std::ios::out | std::ios::binary);
  data.save(out);
}
#ifdef USE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

void covert_images(const char *name, cv::Mat &img_float)
{

  cv::Mat image;
  image = cv::imread(name, 1);
  if (image.empty())
  {
    printf("read file %s fail \n", name);
    exit(-1);
  }
  cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
  int imgW = image.cols;
  int imgH = image.rows;
  float scale = imgH / 32.f;
  int w = (int)(imgW / scale);

  cv::resize(image, image, cv::Size(w, 32));
  img_float = image;
  // image.convertTo(img_float, CV_32F, 1.0/255);
  // img_float = (img_float - 0.5)/0.5;
}
#endif

tnn::tensor_uint8 load_image(const char *filename, bool isjpg, bool normalized) {

  if(isjpg) {
#ifdef USE_OPENCV
    cv::Mat img_float;
    covert_images(filename, img_float);
    uint8_t *data = img_float.ptr<uint8_t>();

    printf("Image load from file %d %d %d\n", img_float.rows, img_float.cols, img_float.channels());
    tnn::tensor_uint8 sample({1, 1,
            (long unsigned int)img_float.rows,
            (long unsigned int)img_float.cols});
    memcpy(sample.get_raw(), data, sizeof(uint8_t) * img_float.cols * img_float.rows);
    return sample;
#else
    printf("** Error: unsupport opencv\n");
    tnn::tensor_uint8 sample;
    return sample;
#endif
  } else {
    // load raw feature
    std::ifstream in(filename, std::ios::in | std::ios::ate | std::ios::binary);
    if (!in) {
      std::cerr << "feature: failed to open raw feature file \"" << filename << "\"" << std::endl;
      std::exit(1);
    }
    if(normalized) {
      // raw is normalled float data
      std::size_t n = in.tellg() / sizeof(float) / 32;
      if (!n || (std::size_t) in.tellg() != sizeof(float) * 32 * n) {
        std::cerr << "feature: invalid size of raw feature file \"" << filename << "\"" << std::endl;
        std::exit(1);
      }
      in.seekg(0);
      tnn::tensor<> sample{1, 1, 32, n};
      sample.load(in);
      in.close();
      tnn::tensor_uint8 sampleA(sample.shape());

      for(int y = 0;y < sampleA.shape(2);y++){
        for(int x = 0;x < sampleA.shape(3);x++){
          sampleA.at(0,0,y,x) = (sample.at(0,0,y,x) + 1.0) * (255 / 2.0f);
        }
      }
      return sampleA;
    } else {
      std::size_t n = in.tellg() / sizeof(uint8_t) / 32;
      if (!n || (std::size_t) in.tellg() != sizeof(uint8_t) * 32 * n) {
        std::cerr << "feature: invalid size of raw feature file \"" << filename << "\"" << std::endl;
        std::exit(1);
      }
      in.seekg(0);
      tnn::tensor_uint8 sampleA{1, 1, 32, n};
      sampleA.load(in);
      in.close();
      return sampleA;
    }
  }
}

void dumpPSNR(const tnn::tensor<>& I1, const tnn::tensor<> &I2)
{
  tnn::tensor<> noise(I1.shape()),signal(I1.shape());
  noise.dumpShape();
  for(int i = 0;i < I1.size();i++){
    float f = I1.at(i) - I2.at(i);

    noise.at(i) = f * f;
    signal.at(i) = I2.at(i) * I2.at(i);
  }
  for(int i = 0;i < I1.shape(1);i++){
    float n = 0;
    float s = 0;
    for(int j = 0;j < I1.shape(2);j++){
      n += noise.at(0,i,j);
      s += signal.at(0,i,j);
    }
    if(n == 0)
      printf("SNR: OK\n");
    else
      printf("SNR: %f\n",20.0 * log10(s/n));
  }
}

#include <float.h>
int decode(const long long *result_array, int size);
void decode_preds(tnn::tensor<float> &preds) {
  size_t features = preds.shape(1);
  size_t preds_num = preds.shape(2);

  long long *preds_indexs = (long long*)malloc(features * sizeof(long long));

  float max = -FLT_MAX;
  float *preds_p = preds.get_raw();

  for (size_t f = 0; f < features; f++) {
    long long i = 0;
    for (size_t p = 0; p < preds_num; p++) {
      if (preds.at(0, f, p) > max) {
        max = preds.at(0, f, p);
        i = p;
      }
    }
    preds_indexs[f] = i;
    max = -FLT_MAX;
  }
  decode(preds_indexs, (int)features);
}

int main(int argc, const char *argv[])
{
  std::chrono::high_resolution_clock::time_point begin, end, forward_begin, total_begin = std::chrono::high_resolution_clock::now();

  program_options options = parse_args(argc, argv);
  if (options.verbose)
    print_options(options);

  tnn::thread_pool threads(options.threads_num);

  std::shared_ptr<tnn::layer > lstm, pca;
  if (options.lstm) {
    begin = std::chrono::high_resolution_clock::now();
    lstm = load_lstm(options.lstm);
    end = std::chrono::high_resolution_clock::now();
    if (options.verbose)
      std::cout << "Lstm loaded.\t" << (end - begin) << "\n";
  }

  forward_begin = std::chrono::high_resolution_clock::now();
  // open out stream

  if (options.lstm) {
    // load sample
    tnn::tensor_uint8 sampleA;
    //tnn::tensor<> result;

    if (options.image) {
      sampleA = load_image(options.image, options.isjpg, options.normalized);
    } else {
      printf("missing image file\n");
      exit(-1);
    }
    //    save_binary_file("sample.dat", sample);

    // sample.dumpShape();
    tnn::tensor<> result;
    {
      long long start = getSystemTime();
      tnn::tensorX t = (lstm->forward(std::move(sampleA), threads));
      result = t.getTensor();
      long long finish = getSystemTime() - start;
      printf("result shape size %ld , time is %lld ms\n", result.ndim(), finish/1000);
    }

    if(0){
      long long start = getSystemTime();
      tnn::tensorX t = (lstm->forward(std::move(sampleA), threads));
      result = t.getTensor();
      long long finish = getSystemTime() - start;
      printf("result shape size %ld , time is %lld ms\n", result.ndim(), finish/1000);
    }

    if(options.verify){
      const char* fn = options.verify;
      std::ifstream verifyFile(fn, std::ios::in | std::ios::binary);
      tnn::tensor<> vSample(result.shape());
      vSample.load(verifyFile);
      verifyFile.close();
      dumpPSNR(result,vSample);
    }

    std::vector<std::size_t>  shape = result.shape();

    decode_preds(result);
    printf("(");
    for (auto begin = shape.begin(); begin != shape.end(); begin++)
      printf("%ld, ", *begin);
    printf(")\n");

    {
      std::ofstream out;
      if (options.output) {
        if (options.binary) {
          out.open(options.output, std::ios::out | std::ios::binary);
          result.save(out);
        }
        else {
          out.open(options.output, std::ios::out);
          if (!out) {
            std::cerr << "feature: failed to open output file \"" << options.output << "\"" << std::endl;
            std::exit(1);
          }
        }
      }
    }
  }



  end = std::chrono::high_resolution_clock::now();
  if (options.verbose) {
    std::cout << "Forward finished.\t" << (end - forward_begin) << "\n" << std::endl;
    std::cout << "All finished.\t" << (end - total_begin) << "\n" << std::endl;
  }

  return 0;
}



const char *help_str = ""
                                                             "Usage: feature [DATA_OPTIONS]... [OPTION]... FILE...\n"
                                                             "Data options:\n"
                                                             "  -a, --lstm=FILE        binary Lstm data\n"
                                                             "  -m, --image=FILE       raw or jpg input data\n"
                                                             "  -i, --isjpg            input data is jpg file\n"
                                                             "  -n, --normalized       raw input data have been normalized\n"
                                                             "Options:\n"
                                                             "  -a, --lstm=FILE        binary Lstm data\n"
                                                             "  -m, --image=FILE       raw or jpg input data\n"
                                                             "  -i, --isjpg            input data is jpg file\n"
                                                             "  -n, --normalized       raw input data have been normalized\n"
                                                             "  -o, --output=FILE      set output file\n"
                                                             "  -b, --binary           set output mode to binary\n"
                                                             "  -t, --threads=NUM      create NUM worker threads\n"
                                                             "  -v, --verbose          enable verbose mode\n"
                                                             "  -h, --help             print this help message\n"
                                                             "Forward flow:\n"
                                                             "                Lstm        PCA\n"
                                                             "             X ---------> Y ---------> Z\n"
                                                             "\n"
                                                             "At least one data option should be present to run this program. And forward\n"
                                                             "flow is changed according to data options. Batch size and extra files is\n"
                                                             "ignored in \"Y -> Z\" mode.\n"
                                                             ;

program_options parse_args(int argc, const char *argv[]) {
  program_options options {
    nullptr, nullptr, nullptr, nullptr,nullptr,
        false, false, false, false,
        std::thread::hardware_concurrency(), std::thread::hardware_concurrency(),
    {}
  };
  const char *temp_str, *char_p;
  int temp_int;

  for (int i = 1; i < argc; ++i) {
    int sh = 1;
    if (!std::strcmp(argv[i], "-v") || (!std::strncmp(argv[i], "--verify=", 7) && sh--)) {
      if (sh) {
        if (++i == argc) {
          std::cerr << "requires path to verify data after \"-v\"" << std::endl;
          std::exit(1);
        }
        options.verify = argv[i];
      } else
        options.lstm = argv[i] + 7;
    }else if (!std::strcmp(argv[i], "-a") || (!std::strncmp(argv[i], "--lstm=", 7) && sh--)) {
      if (sh) {
        if (++i == argc) {
          std::cerr << "feature: requires path to Lstm data after \"-a\"" << std::endl;
          std::exit(1);
        }
        options.lstm = argv[i];
      } else
        options.lstm = argv[i] + 7;
    }else  if (!std::strcmp(argv[i], "-m") || (!std::strncmp(argv[i], "--image=", 8) && sh--)) {
      if (sh) {
        if (++i == argc) {
          std::cerr << "feature: requires path to Lstm data after \"-i\"" << std::endl;
          std::exit(1);
        }
        options.image = argv[i];
      } else
        options.image = argv[i] + 8;

    }else  if (!std::strcmp(argv[i], "-n") || (!std::strcmp(argv[i], "--normalized="))) {
        options.normalized = true;
    } else if (!std::strcmp(argv[i], "-p") || (!std::strncmp(argv[i], "--pca=", 6) && sh--)) {
      if (sh) {
        if (++i == argc) {
          std::cerr << "feature: requires path to PCA data after \"-p\"" << std::endl;
          std::exit(1);
        }
        options.pca = argv[i];
      } else
        options.pca = argv[i] + 6;
    } else if (!std::strcmp(argv[i], "-o") || (!std::strncmp(argv[i], "--output=", 9) && sh--)) {
      if (sh) {
        if (++i == argc) {
          std::cerr << "feature: requires path to output file after \"-o\"" << std::endl;
          std::exit(1);
        }
        options.output = argv[i];
      } else
        options.output = argv[i] + 9;
    } else if (!std::strcmp(argv[i], "-t") || (!std::strncmp(argv[i], "--threads=", 10) && sh--)) {
      if (sh) {
        if (++i == argc) {
          std::cerr << "feature: requires number of threads after \"-t\"" << std::endl;
          std::exit(1);
        }
        temp_str = argv[i];
      } else
        temp_str = argv[i] + 10;
      for (char_p = temp_str; *char_p && *char_p >= '0' && *char_p <= '9'; ++char_p);
      if (!*temp_str || *char_p || (temp_int = std::atoi(temp_str)) < 1) {
        std::cerr << "feature: invalid number of threads" << std::endl;
        std::exit(1);
      }
      options.threads_num = temp_int;
    } else if (!std::strcmp(argv[i], "-s") || (!std::strncmp(argv[i], "--batch=", 8) && sh--)) {
      if (sh) {
        if (++i == argc) {
          std::cerr << "feature: requires number of batch size after \"-t\"" << std::endl;
          std::exit(1);
        }
        temp_str = argv[i];
      } else
        temp_str = argv[i] + 8;
      for (char_p = temp_str; *char_p && *char_p >= '0' && *char_p <= '9'; ++char_p);
      if (!*temp_str || *char_p || (temp_int = std::atoi(temp_str)) < 1) {
        std::cerr << "feature: invalid number of batch size" << std::endl;
        std::exit(1);
      }
      options.batch_size = temp_int;
    } else if (!std::strcmp(argv[i], "-b") || !std::strcmp(argv[i], "--binary")) {
      options.binary = true;
    } else if (!std::strcmp(argv[i], "-i") || !std::strcmp(argv[i], "--isjpg")) {
      options.isjpg = true;
    } else if (!std::strcmp(argv[i], "-v") || !std::strcmp(argv[i], "--verbose")) {
      options.verbose = true;
    } else if (!std::strcmp(argv[i], "-h") || !std::strcmp(argv[i], "--help")) {
      std::cout << help_str << std::endl;
      std::exit(0);
    } else if (argv[i][0] == '-') {
      std::cerr << "feature: unrecognized option \"" << argv[i] << "\"" << std::endl;
      std::exit(1);
    } else
      options.files.push_back(argv[i]);
  }
  if (!options.lstm && !options.pca) {
    std::cerr << "feature: requires at least one data option" << std::endl;
    exit(1);
  }
  // if (options.files.empty()) {
  //     std::cerr << "feature: requires at least one input file" << std::endl;
  //     std::exit(1);
  // }
  return options;
}

void print_options(const program_options &options) {
  std::cout << "Options:\n";
  if (options.lstm && options.pca)
    std::cout << "  Forward flow:       X -> Y -> Z\n";
  else if (options.lstm)
    std::cout << "  Forward flow:       X -> Y\n";
  else if (options.pca)
    std::cout << "  Forward flow:       Y -> Z\n";
  if (options.lstm)
    std::cout << "  Lstm data:       \"" << options.lstm << "\"\n";
  if (options.pca)
    std::cout << "  PCA data:           \"" << options.pca << "\"\n";
  if (options.output)
    std::cout << "  Output file:        \"" << options.output << "\"\n";
  else
    std::cout << "  Output file:        stdout\n";
  if (options.binary)
    std::cout << "  Output mode:        binary\n";
  else
    std::cout << "  Output mode:        text\n";
  if (options.lstm) {
    std::cout << "  Files num:          " << options.files.size() << "\n";
    std::cout << "  Batch size:         " << options.batch_size <<"\n";
  }
  std::cout << "  Threads num:        " << options.threads_num <<"\n";
  std::cout << "  AVX2 enabled:       " << std::boolalpha << AVX_ENABLED << "\n";
  std::cout << std::endl;
}



template <typename Rep, typename Period>
std::ostream &operator << (std::ostream &out, const std::chrono::duration<Rep, Period> &duration) {
  double nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  std::size_t precision = out.precision(5);
  if (nanoseconds > 1e9)
    out << nanoseconds / 1e9 << "s";
  else if (nanoseconds > 1e6)
    out << nanoseconds / 1e6 << "ms";
  else if (nanoseconds > 1e3)
    out << nanoseconds / 1e3 << "us";
  else
    out << nanoseconds << "ns";
  out.precision(precision);
  return out;
}

void save_result(std::ostream &out, const tnn::tensor<> &result, bool binary) {
  assert(result.ndim() == 2);
  if (binary)
    result.save(out);
  else {
    for (std::size_t i = 0; i < result.shape(0); ++i) {
      out << result.at(i, 0);
      for (std::size_t j = 1; j < result.shape(1); ++j)
        out << " " << result.at(i, j);
      out << "\n";
    }
  }
}
