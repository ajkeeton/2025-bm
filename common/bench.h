#pragma once

class bench_t {
public:
    bench_t() {
        init("bench");
    }

    bench_t(const char *prefix) {
        init(prefix);
    }

    void init(const char *prefix) {
        t_last_log = millis();
        t_last_update = micros();
        strncpy(log_prefix, prefix, sizeof(log_prefix) - 1);
        log_prefix[sizeof(log_prefix) - 1] = '\0';
    }

    void next() {
        uint64_t now = micros();
        uint64_t d = now - t_last_update; 
        t_sum += d;
        n_samples++;
        t_last_update = now;
    }

    void log() {
        uint32_t now = millis();
        if(now - t_last_log < t_last_log || !n_samples)
            return;

        log_force();
    }

    void log_force() {
        if(!n_samples)
            return;
        t_last_log = millis();
        Serial.printf("%s: %.4f ms\n", log_prefix, t_sum / (float)n_samples / 1000.0);
        n_samples = 0;
        t_sum = 0;
    }

    uint32_t n_samples = 0;
    uint64_t t_last_log = 1000,
             t_last_update = 0,
             t_sum = 0;

    char log_prefix[32] = "bench";
};