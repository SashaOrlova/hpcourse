#include <pthread.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <unistd.h>

using namespace std;
pthread_cond_t condition_value_change;
pthread_cond_t condition_value_change_finish;
pthread_mutex_t condition_value_change_mutext;
pthread_barrier_t threads_start_barrier;
int value;
bool has_value = false;
bool is_finish_reading = false;
int sleep_time;
int N = 0;

void* producer_routine(void* arg) {
    string str;
    pthread_barrier_wait(&threads_start_barrier);
    getline(cin, str);
    stringstream stream;
    stream << str;
    int i;
    pthread_mutex_lock(&condition_value_change_mutext);
    while (stream >> i) {
        *((int*)arg) = i;
        has_value = true;
        pthread_cond_signal(&condition_value_change);
        pthread_cond_wait(&condition_value_change_finish, &condition_value_change_mutext);
    }
    is_finish_reading = true;
    pthread_mutex_unlock(&condition_value_change_mutext);
    pthread_cond_broadcast(&condition_value_change);
    pthread_exit(NULL);
}

thread_local int part_sum = 0;
void* consumer_routine(void* arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_barrier_wait(&threads_start_barrier);
    while (true) {
        pthread_mutex_lock(&condition_value_change_mutext);
        if (is_finish_reading) {
            pthread_mutex_unlock(&condition_value_change_mutext);
            pthread_exit((void*)part_sum);
        }
        if (!has_value) {
            pthread_cond_wait(&condition_value_change, &condition_value_change_mutext);
        }
        if (is_finish_reading) {
            pthread_mutex_unlock(&condition_value_change_mutext);
            pthread_exit((void*)part_sum);
        } else {
            has_value = false;
            part_sum += *((int*)arg);
            *((int*)arg) = 0;
        }
        pthread_mutex_unlock(&condition_value_change_mutext);
        pthread_cond_signal(&condition_value_change_finish);
        if (sleep_time) {
            sleep(rand() % sleep_time);
        }
    }
}

void* consumer_interruptor_routine(void* arg) {
    pthread_barrier_wait(&threads_start_barrier);
    pthread_t* threads = (pthread_t*)arg;
    while (true) {
        int index = rand() % N;
        pthread_cancel(threads[index]);
        if (is_finish_reading) {
            pthread_exit(NULL);
        }
    }
}

int run_threads(int consumers_number) {
    pthread_t threads[consumers_number];
    int res[consumers_number];

    pthread_mutex_init(&condition_value_change_mutext, NULL);
    pthread_cond_init(&condition_value_change, NULL);
    pthread_cond_init(&condition_value_change_finish, NULL);
    pthread_barrier_init(&threads_start_barrier, NULL, consumers_number + 2);

    pthread_t producer;
    pthread_t interruptor;

    pthread_create( &producer, NULL, producer_routine, &value);
    pthread_create( &interruptor, NULL, consumer_interruptor_routine, &threads);
    for (int i = 0; i < consumers_number; i++) {
        pthread_create(&(threads[i]), NULL, consumer_routine, &value);
    }

    for (int i = 0; i < consumers_number; i++) {
        pthread_join(threads[i], (void**)&(res[i]));
    }

    pthread_join(producer, NULL);
    pthread_join(interruptor, NULL);
    pthread_mutex_destroy(&condition_value_change_mutext);
    pthread_cond_destroy(&condition_value_change);
    pthread_cond_destroy(&condition_value_change_finish);
    pthread_barrier_destroy(&threads_start_barrier);

    int sum = 0;
    for (int i = 0 ; i < consumers_number; i++) {
        sum += res[i];
    }

    return sum;
}

int main(int argc, char **argv) {
    N = stoi(argv[1]);
    sleep_time = stoi(argv[2]);
    cout << run_threads(N) << endl;
    return 0;
}
