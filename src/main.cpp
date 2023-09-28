#ifndef MAIN
#define MAIN

#include "ukf.h"
#include <fstream>
#include "matrix_utils.h"
#include "bayes_runner.h"
#include <iostream>
#include <filesystem>
#include "detailed_run.h"

std::pair<std::string, std::string> copy_base_config(std::string base_path, const int &age) {
    std::string base_blood_path = base_path + "/data/configs/base_configs/blood_config_" + std::to_string(age) + ".json";
    std::string base_heart_path = base_path + "/data/configs/base_configs/heart_config_" + std::to_string(age) + ".json";

    std::string current_blood_path = base_path + "/data/configs/current_configs/blood_config_" + std::to_string(age) + ".json";
    std::string current_heart_path = base_path + "/data/configs/current_configs/heart_config_" + std::to_string(age) + ".json";

    std::filesystem::copy_file(base_blood_path, current_blood_path);
    std::filesystem::copy_file(base_heart_path, current_heart_path);

    return std::make_pair(current_blood_path, current_heart_path);
}

void clear_current_configs(std::string base_path, const int &age) {
    std::filesystem::remove(base_path + "/data/configs/current_configs/blood_config_" + std::to_string(age) + ".json");
    std::filesystem::remove(base_path + "/data/configs/current_configs/heart_config_" + std::to_string(age) + ".json");
}

void run_ukf(std::string base_path, const int &age, const double &SBP, const double &DBP, const double &SV, const double &HR) {
    std::pair<std::string, std::string> current_paths(copy_base_config(base_path, age));
    set_hr(HR, current_paths.second);

    Ukf filter(SBP, DBP, SV, current_paths.first, current_paths.second, base_path);
    filter.execute_pipeline();

    clear_current_configs(base_path, age);
}

void run_backup_ukf(std::string base_path, const int &age, const double &SBP, const double &DBP, const double &SV, const double &HR) {
    std::pair<std::string, std::string> current_paths(copy_base_config(base_path, age));
    set_hr(HR, current_paths.second);

    Ukf filter(SBP, DBP, SV, current_paths.first, current_paths.second, base_path, true);
    filter.execute_pipeline();

    clear_current_configs(base_path, age);
}

void run_bo(std::string base_path, const int &age, const double &SBP, const double &DBP, const double &SV, const double &HR) {
    std::pair<std::string, std::string> current_paths(copy_base_config(base_path, age));
    set_hr(HR, current_paths.second);

    BayesRunner bo_runner(base_path, current_paths.first, current_paths.second);
    bo_runner.execute_pipeline();

    clear_current_configs(base_path, age);
}

void run_detailed(std::string base_path, Eigen::MatrixXd &Teta, const int &age, const double &HR) {
    std::pair<std::string, std::string> current_paths(copy_base_config(base_path, age));
    set_hr(HR, current_paths.second);

    DetailedRun runner(base_path, current_paths.first, current_paths.second, Teta, age, HR);
    runner.run_task();

    clear_current_configs(base_path, age);
}

int main(int argc, char *argv[]) {
    std::string base_path = argv[0];
    base_path = base_path.substr(0, base_path.find_last_of('/'));
    base_path = base_path.substr(0, base_path.find_last_of('/'));

    std::filesystem::remove_all(base_path + "/data/configs/current_configs");
    std::filesystem::remove_all(base_path + "/data/out");
    std::filesystem::create_directory(base_path + "/data/configs/current_configs");
    std::filesystem::create_directory(base_path + "/data/out");

    std::string mode = argv[1];

    if (mode == "-manual") {
        std::cout << atoi(argv[2]) << "," << atof(argv[3]) << "," << atof(argv[4]) << "," << atof(argv[5]) << "," << atof(argv[6]) << std::endl;
        run_ukf(base_path, atoi(argv[2]), atof(argv[3]), atof(argv[4]), atof(argv[5]), atof(argv[6]));
    }
    else if (mode == "-config") {
        const int num_tests = atoi(argv[2]);
        const int target_vars = 5;
        Eigen::MatrixXd test_matrix;
        std::ifstream test_file(base_path + "/data/configs/run_config.csv");
        read_csv_matrix(test_file, test_matrix, num_tests, target_vars);
        for (int i = 0; i < num_tests; ++i) {
            std::cout << "new case!" << std::endl;
            std::cout << test_matrix(i, 0) << "," << test_matrix(i, 1) << "," << test_matrix(i, 2) << "," << test_matrix(i, 3) << "," << test_matrix(i, 4) << std::endl;
            run_ukf(base_path, (int)(test_matrix(i, 0)), test_matrix(i, 1), test_matrix(i, 2), test_matrix(i, 3), test_matrix(i, 4));
        }
    }
    else if (mode == "-detailed") {
        int age = atoi(argv[2]);
        double HR = atof(argv[3]);
        std::string path_to_params = argv[4];
        std::ifstream param_file(base_path + "/" + path_to_params);
        Eigen::MatrixXd Teta_dynamic;
        read_csv_matrix(param_file, Teta_dynamic, 7, 1);
        std::cout << age << "," << Teta_dynamic << std::endl;
        run_detailed(base_path, Teta_dynamic, age, HR);
    }
    else if (mode == "-bayes") {
        std::cout << "bayesian test!" << std::endl;
        std::cout << atoi(argv[2]) << "," << atof(argv[3]) << "," << atof(argv[4]) << "," << atof(argv[5]) << "," << atof(argv[6]) << std::endl;
        run_bo(base_path, atoi(argv[2]), atof(argv[3]), atof(argv[4]), atof(argv[5]), atof(argv[6]));
    }
    else if (mode == "-ukf_back"){
        // age,SBP,DBP,SV,HR
        std::cout << atoi(argv[2]) << "," << atof(argv[3]) << "," << atof(argv[4]) << "," << atof(argv[5]) << "," << atof(argv[6]) << std::endl;
        run_backup_ukf(base_path, atoi(argv[2]), atof(argv[3]), atof(argv[4]), atof(argv[5]), atof(argv[6]));
    }
    else {
        std::cout << "ERR: Choose option" << std::endl;
    }
}

#endif
