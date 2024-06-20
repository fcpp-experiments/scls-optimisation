// Copyright © 2024 Giorgio Audrito. All Rights Reserved.

/**
 * @file batch.cpp
 * @brief Runs multiple executions non-interactively from the command line, producing overall plots.
 */

#include "lib/setup.hpp"

using namespace fcpp;

int main() {
    //! @brief Construct the plotter object.
    option::plot_t p;
    //! @brief The component type (batch simulator with given options).
    using comp_t = component::batch_simulator<option::list>;
    //! @brief The list of initialisation values to be used for simulations.
    auto init_list = batch::make_tagged_tuple_sequence(
        batch::arithmetic<option::seed >(0, 9, 1),      // 10 different random seeds
        batch::arithmetic<option::speed>(0, 48, 2, 10), // 25 different speeds
        batch::arithmetic<option::dens >(5, 29, 1, 10), // 25 different densities
        batch::arithmetic<option::hops >(1, 25, 1, 10), // 25 different hop sizes
        batch::arithmetic<option::tvar >(0, 48, 2, 10), // 25 different time variances
        // generate output file name for the run
        batch::stringify<option::output>("output/batch", "txt"),
        // computes side length from hops
        batch::formula<option::side, size_t>(option::side_formula{}),
        // computes device number from dens and side
        batch::formula<option::devices, size_t>(option::device_formula{}),
        batch::constant<option::plotter>(&p) // reference to the plotter object
    );
    //! @brief Runs the given simulations.
    batch::run(comp_t{}, init_list);
    //! @brief Builds the resulting plots.
    std::cout << plot::file("batch", p.build());
    return 0;
}
