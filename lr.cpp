#include <iostream>
#include <vector>
#include <utility>
#include <fstream>
#include <cstring>
#include <cmath>
#ifdef _WIN32
#include "windows.h"
#endif

using namespace std;

#define WORD_SIZE 128
#define LINE_WIDTH 12800

#define TARGET_ACCURACY 0.99
#define FEATURE_DIM 30
#define BATCH_SIZE 256
#define BATCH_TOTAL 100
#define ERROR_LAST_ITERATION 100
#define LEARNING_RATE 0.01
#define LINES_TO_READ 977
#define TRAIN_RECORD_NUM 777
#define TEST_RECORD_NUM 200


// MACRO & FUNCTIONS
#define F_SIGMOID( x ) ( 1.0 / ( 1.0 + expf( -x ) ) )

float cpu_dot(const vector< pair<int, float> > & a, const vector<float> & b) {
	float ret = 0.0;
	for (vector< pair<int, float> >::const_iterator i = a.begin();
		i != a.end(); i++) {
		ret += i->second * b[i->first];
	}
	return ret;
}

vector<float> cpu_batch_dot(const vector< vector< pair<int, float> > > & data, const vector<float> & b) {
	vector<float> rets(data.size(), 0);
	for (int i = 0; i < data.size(); i++) {
		rets[i] = cpu_dot(data[i], b);
	}
	return rets;
}

double sigmoid(float x) {
	return 1.0 / (1.0 + exp(-1.0 * x));
}

double cpu_grad(const vector< pair<int, float> > & x,
	const float wtx,
	const int label,
	vector<float> & w,
	const float learning_rate,
	const float lambda) {
	//float err = (float)label - sigmoid(wtx);
	float err = (float)label - F_SIGMOID( wtx );
	for (vector< pair<int, float> >::const_iterator i = x.begin();
		i != x.end(); i++) {
		w[i->first] += learning_rate * (err - lambda * w[i->first]);
	}
	return abs(err);
}

double cpu_batch_grad(const vector< vector< pair<int, float> > > & data,
	const vector< int > & label,
	vector<float> & b,
	const float learning_rate,
	const float lambda) {
	vector<float> dot = cpu_batch_dot(data, b);
	float err = 0.;
	float total = 0.;
	for (int i = 0; i < data.size(); i++) {
		err += cpu_grad(data[i], dot[i], label[i], b, learning_rate, lambda);
		total += 1.;
	}
	return err / total;
}


void mock_sample(const int max_feature_id, vector< pair<int, float> > & out, int * label) {
	int base = (int)floor(0.2*max_feature_id);
	int count = rand() % (1-base) + base;
	int ret = 0;
	for(int i = 0; i < count; i++) {
		int fid = rand() % max_feature_id;
		if(fid % 2 == 0) ret += 1;
		else ret -= 1;
		if(abs(ret) > 10) break;
		out.push_back(make_pair<int, float>(fid, 1.0));
	}
	*label = (ret > 0) ? 1 : 0;
}


// get data from dataset
void prepare_sample_batch( void * data, vector< pair<int, float> > & out, int * label ) {
    float * mat = (float*) data;
    // get input vector
    int i;
    int j = rand() % TRAIN_RECORD_NUM;
    for ( i=0; i<FEATURE_DIM-1; i++ ) {
        if ( mat[j * FEATURE_DIM + i] != 0 ) {
            out.push_back( make_pair<int, float>(i, mat[j * FEATURE_DIM + i] ) );
        }
    }
    // add bias
    out.push_back( make_pair<int, float>( FEATURE_DIM - 1, 1.0 ) );
    *label =mat[j * FEATURE_DIM + FEATURE_DIM - 1];
}


void cpu_lr ( void * data ) {
	float learning_rate = LEARNING_RATE;
	float lambda = 0.00;
	float err;
	float err_hist[ERROR_LAST_ITERATION]; // error last for a low level
	int err_id = 0;
	float err_tot = 0;
	for(int i=0; i<ERROR_LAST_ITERATION; i++)
	{
		err_hist[i] = 1.0;
		err_tot += err_hist[i];
	}
	vector<float> model(FEATURE_DIM, 0);
	// initialize model
	for (int i = 0; i < model.size(); i++) {
		model[i] = 0.5 - (double)(rand() % 10000) / 10000.0;
		//model[i] = 1.0;
	}
	// mini-batch algorithm with steepest-descent method
	size_t i, j;
	int l;
	for (i = 0; i < BATCH_TOTAL; i++) {
		vector< vector< pair<int, float> > > batch_data;
		vector< int > batch_label;
		for (j = 0; j < BATCH_SIZE; j++) {
			vector< pair<int, float> > x;
            prepare_sample_batch( data, x, &l );
			batch_data.push_back(x);
			batch_label.push_back(l);
		}
		// caculate the error
		err = cpu_batch_grad(batch_data, batch_label, model,
			learning_rate, lambda);
		cout << "iter#" << i << " mean error: " << err << endl;

		// update error history
		err_tot -= err_hist[err_id];
		err_tot += err;
		err_hist[err_id] = err;
		err_id = (err_id+1)%ERROR_LAST_ITERATION;

		if (err_tot/ERROR_LAST_ITERATION < 1.0 - TARGET_ACCURACY) {
			std::cout << "Target accuracy achieved!\n" << std::endl;
			break;
		}
	}
	std::cout << "final model accuracy is: " << 1.0-err_tot/ERROR_LAST_ITERATION << std::endl;
	// print weights
	for (i = 0; i < model.size(); i++)
		std::cout << model[i] << " ";
	std::cout << std::endl;
}



// split line into vectors of words
void split_line(
        std::vector< std::string > & words,
        const char * str,
        char delim ) {
    int i, j;
    char w[WORD_SIZE];
    words.clear();
    i = j = 0;
    for( ; i<LINE_WIDTH; i++ ) {
        if ( !str[i] ) {
            w[j] = 0;
            words.push_back( std::string( w ) );
            break;
        } else if ( str[i] == delim ) {
            w[j] = 0;
            words.push_back( std::string( w ) );
            j = 0;
        } else if ( j == WORD_SIZE - 1 ) {
            std::cout << "word size exceeds maxium limit!" << std::endl;
            exit(1);
        } else {
            w[j++] = str[i];
        }
    }
}


// read data from csv file and convert it into sparsed data format
void read_data( const char * file, void ** data ) {
    // last stop date
    std::ifstream strm;
    int i, j, k;
    char str[LINE_WIDTH];
    std::vector< std::string > names;
    std::vector< std::string > values;
    float * mat = new float[FEATURE_DIM * LINES_TO_READ];
    float v;

    *data = mat;

    strm.open( file );
    if ( !strm.is_open() ) {
        std::cout << "failed to open data file!" << std::endl;
        exit(1);
    }

    // read header
    strm.getline( str, sizeof(str)-1 );
    split_line( names, str, ',' );

    // read body
    for ( j=0; j<LINES_TO_READ; j++ ) {

        strm.getline( str, sizeof(str)-1 );
        split_line( values, str, ',' );

        for ( i=0, k=0; i<values.size(); i++ ) {

            k++;

            if ( names[i] == "LAST_STOP_DATE" ) {

                v = atof( values[i].c_str() );

                if ( v <= 0 || v > 12 ) {
                    mat[j * FEATURE_DIM + k - 1 ] = 13;
                }

                mat[j * FEATURE_DIM + k - 1 ] = v;

            } else if ( names[i] == "INNET_MONTHS" ) {

                v = atof( values[i].c_str() );

                if ( v <=0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 148 ) {
                    mat[j * FEATURE_DIM + k - 1] = logf(149);
                } else {
                    mat[j * FEATURE_DIM + k - 1] = logf( v );
                }

            } else if ( names[i] == "TOTAL_FLUX" ) {

                v = atof( values[i].c_str() );

                if ( v <=0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 1373 ) {
                    mat[j * FEATURE_DIM + k - 1] = logf(1374);
                } else {
                    mat[j * FEATURE_DIM + k - 1] = logf( v );
                }

            } else if ( names[i] == "LOCAL_FLUX_ZB" ) {

                v = atof( values[i].c_str() );
                mat[j * FEATURE_DIM + k - 1] = v;

            } else if ( names[i] == "JF_TIMES" ) {

                v = atof( values[i].c_str() );

                if ( v <= 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 1502 ) {
                    mat[j * FEATURE_DIM + k - 1] = logf(1503);
                } else {
                    mat[j * FEATURE_DIM + k - 1] = logf( v );
                }

            } else if ( names[i] == "NOROAM_LONG_JF_TIMES" ) {

                v = atof( values[i].c_str() );

                if ( v <= 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 386 ) {
                    mat[j * FEATURE_DIM + k - 1] = logf(387);
                } else {
                    mat[j * FEATURE_DIM + k - 1] = logf( v );
                }

            } else if ( names[i] == "ROAM_ZB" ) {

                v = atof( values[i].c_str() );
                mat[j * FEATURE_DIM + k - 1] = v;

            } else if ( names[i] == "ZHUJIAO_ZB" ) {

                v = atof( values[i].c_str() );
                mat[j * FEATURE_DIM + k - 1] = v;

            } else if ( names[i] == "TOLL_NUMS_ZB" ) {

                v = atof( values[i].c_str() );
                mat[j * FEATURE_DIM + k - 1] = v;

            } else if ( names[i] == "ACCT_FEE" ) {

                v = atof( values[i].c_str() );

                if ( v <= 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 170 ) {
                    mat[j * FEATURE_DIM + k - 1] = logf(171);
                } else {
                    mat[j * FEATURE_DIM + k - 1] = logf( v );
                }

            } else if ( names[i] == "ROAM_VOICE_FEE" ) {

                v = atof( values[i].c_str() );

                if ( v <= 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 49 ) {
                    mat[j * FEATURE_DIM + k - 1] = logf(50);
                } else {
                    mat[j * FEATURE_DIM + k - 1] = logf( v );
                }

            } else if ( names[i] == "ZENGZHI_FEE" ) {

                v = atof( values[i].c_str() );

                if ( v < 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 4 ) {
                    mat[j * FEATURE_DIM + k - 1] = 5;
                } else {
                    mat[j * FEATURE_DIM + k - 1] = v;
                }

            } else if ( names[i] == "OWE_FEE" ) {

                v = atof( values[i].c_str() );

                if ( v <= 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 125 ) {
                    mat[j * FEATURE_DIM + k - 1] = logf(126);
                } else {
                    mat[j * FEATURE_DIM + k - 1] = logf( v );
                }

            } else if ( names[i] == "FLUX_TIME" ) {

                v = atof( values[i].c_str() );

                if ( v <= 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 47292 ) {
                    mat[j * FEATURE_DIM + k - 1] = logf(47293);
                } else {
                    mat[j * FEATURE_DIM + k - 1] = logf( v );
                }

            } else if ( names[i] == "YQ_OWE_MONTHS" ) {

                v = atof( values[i].c_str() );

                if ( v < 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 1;
                } else if ( v > 6 ) {
                    mat[j * FEATURE_DIM + k - 1] = 7;
                } else {
                    mat[j * FEATURE_DIM + k - 1] = v;
                }

            } else if ( names[i] == "VAR_CDR_NUM" ) {

                v = atof( values[i].c_str() );
                mat[j * FEATURE_DIM + k - 1] = v;

            } else if ( names[i] == "CALL_DAYS" ) {

                v = atof( values[i].c_str() );
                mat[j * FEATURE_DIM + k - 1] = v;

            } else if ( names[i] == "LAST_CALL_TIME" ) {

                v = atof( values[i].c_str() );

                if ( v < 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 18 ) {
                    mat[j * FEATURE_DIM + k - 1] = 19;
                } else {
                    mat[j * FEATURE_DIM + k - 1] = v;
                }

            } else if ( names[i] == "CALL_DURA_LAST7_CN" ) {

                v = atof( values[i].c_str() );

                if ( v <= 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 437 ) {
                    mat[j * FEATURE_DIM + k - 1] = logf(438);
                } else {
                    mat[j * FEATURE_DIM + k - 1] = logf( v );
                }

            } else if ( names[i] == "CELLID_NUM" ) {

                v = atof( values[i].c_str() );

                if ( v <= 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v > 86 ) {
                    mat[j * FEATURE_DIM + k - 1] = logf( 87 );
                } else {
                    mat[j * FEATURE_DIM + k - 1] = logf( v );
                }

            } else if ( names[i] == "PAY_MODE" ) {

                v = atoi ( values[i].c_str() );

                if ( v == 1 ) {
                    mat[j * FEATURE_DIM + k - 1] = 1;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v == 2) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 1;
                } else {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                }

            } else if ( names[i] == "IS_GRP_MBR" ) {

                v = atoi ( values[i].c_str() );

                if ( v == 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 1;
                } else {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                }

            } else if ( names[i] == "IS_TERM_IPHONE" ) {

                v = atoi ( values[i].c_str() );

                if ( v == 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 1;
                } else {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                }

            } else if ( names[i] == "IS_USE_SMART" ) {

                v = atoi ( values[i].c_str() );

                if ( v == 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 1;
                } else {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                }

            } else if ( names[i] == "NET_TYPE" ) {

                v = atoi ( values[i].c_str() );

                if ( v == 0 ) {
                    mat[j * FEATURE_DIM + k - 1] = 1;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v == 1 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 1;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v == 2 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 1;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                } else if ( v == 3 ) {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 1;
                } else {
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                    k++;
                    mat[j * FEATURE_DIM + k - 1] = 0;
                }

            } else {
                k--;
                // fill in flags
                if ( names[i] == "STABLE_FLAG" ) {
                    v = atoi( values[i].c_str() );
                    mat[j * FEATURE_DIM + FEATURE_DIM - 1] = v;
                }
            }
        }
    }
}


void clear_data( void ** data ) {
    delete (float*)(*data);
    *data = NULL;
}

int main() {
#ifdef _WIN32
	LARGE_INTEGER begin;
	LARGE_INTEGER end;
	LARGE_INTEGER freq;
	QueryPerformanceFrequency( &freq );
	QueryPerformanceCounter( &begin );
#endif
    // read data
    void * data;
    read_data( "_2G_FILTERED.CSV", &data );

    // training
	cpu_lr( data );
#ifdef _WIN32
	QueryPerformanceCounter(&end);
	double millsec = 1000.0 * (end.QuadPart - begin.QuadPart)/freq.QuadPart;
	std::cout<<"cost time: "<< millsec << " milliseconds." << std::endl;
	fflush(stdout);
#endif

    // test model


    // clear
    clear_data( &data );

    return 0;
}