#ifndef Fourier_TX_h
#define Fourier_TX_h

#include <iostream>
#include <vector>
#include <cmath>
#include <numbers>
#include <complex>
#include <thread>


#define _PI 3.14159265358979323846

using Complex = std::complex<float>;

class FourierTX {
public:
    void fft(std::vector<Complex> &a) {
        int n = a.size();
        if (n <= 1) return;

        // Bit-reversal permutation
        /*for (int i = 1, j = 0; i < n; ++i) {
            int bit = n >> 1;
            while (j & bit) {
                j ^= bit;
                bit >>= 1;
            }
            j ^= bit;

            if (i < j) {
                std::swap(a[i], a[j]);
            }
        }*/

        // Cooley-Tukey iterative in-place FFT
        for (int len = 2; len <= n; len <<= 1) {
            float angle = -2.0f * static_cast<float>(_PI) / len;
            Complex wlen(std::cos(angle), std::sin(angle));
            for (int i = 0; i < n; i += len) {
                Complex w(1);
                for (int j = 0; j < len / 2; ++j) {
                    Complex u = a[i + j];
                    Complex v = a[i + j + len / 2] * w;
                    a[i + j] = u + v;
                    a[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }
    }

    void fft_chunk(std::vector<Complex> &a, int start, int len, Complex wlen) {
        for (int i = start; i < start + len; i += len) {
            Complex w(1);
            for (int j = 0; j < len / 2; ++j) {
                Complex u = a[i + j];
                Complex v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }

    void multi_fft(std::vector<Complex> &a) {
        int n = a.size();
        if (n <= 1) return;

        std::vector<std::thread> threads;

        // Cooley-Tukey iterative in-place FFT
        for (int len = 2; len <= n; len <<= 1) {
            float angle = static_cast<float>(-2.0f * _PI / len);
            Complex wlen(std::cos(angle), std::sin(angle));
            int num_threads = std::thread::hardware_concurrency();
            int chunk_size = n / num_threads;

            for (int t = 0; t < num_threads; ++t) {
                int start = t * chunk_size;
                int length = (t == num_threads - 1) ? (n - start) : chunk_size;
                threads.emplace_back([this, &a, start, length, wlen]() {
                    this->fft_chunk(a, start, length, wlen);
                    });
            }

            for (auto &th : threads) {
                th.join();
            }
            threads.clear();
        }
    }
    
    void recursive_fft (std::vector<Complex> &a) {
                
        int fft_size = a.size();
                
        // recursion base case
        if (fft_size <= 1) return;
        
        // Split the vector into even and odd indexed elements
        std::vector<Complex> even(fft_size / 2), odd(fft_size / 2);
        for (int i = 0; i < fft_size / 2; ++i) {
            even[i] = a[i * 2];
            odd[i] = a[i * 2 + 1];
        }
        
        // Recursively call fft on the smaller vectors
        fft(even);
        fft(odd);
        
        // Combine the results
        for (int k = 0; k < fft_size / 2; ++k) {
            Complex t = std::polar(1.0f, -2.0f * static_cast<float>(_PI) * k / fft_size) * odd[k];
            a[k] = even[k] + t;
            a[k + fft_size / 2] = even[k] - t;
        }
    }

    void chroma_features (std::vector<float> *input, int window_size) {
        if (input->size() < static_cast<size_t>(window_size)) {
            return;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<Complex> accumulated_results(input->size(), Complex(0, 0));
        for (size_t start=0; start<(input->size()/window_size)-1; start += window_size) {
            
            auto data = std::vector<Complex>(
                input->begin() + start, 
                input->begin() + start + window_size
            );
            
            this->fft(data);
            
            /*for (size_t i = 0; i < input.size(); ++i) {
                accumulated_results[i] += data[i];
            }*/
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        //fprintf(stderr, "\r                    \r%f", duration.count());
    }
};

#endif // Fourier_TX_h