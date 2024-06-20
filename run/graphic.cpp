// Copyright © 2024 Giorgio Audrito. All Rights Reserved.

/**
 * @file graphic.cpp
 * @brief Runs a single execution with a graphical user interface.
 */

#include "lib/setup.hpp"

using namespace fcpp;

int main() {
    using namespace fcpp;

    // The plotter object.
    option::plot_t plotter;
    // The network object type (interactive simulator with given options).
    using net_t = component::interactive_simulator<option::list>::net;
    std::cout << "/*\n";
    {
        // The initialisation values (simulation name, texture of the reference plane, node movement speed).
        auto init_v = common::make_tagged_tuple<option::name, option::speed, option::dens, option::hops, option::tvar, option::side, option::devices, option::plotter>(
            "Optimised implementations of SLCS",
            10,
            10,
            10,
            10,
            0,
            0,
            &plotter
        );
        common::get<option::side>(init_v) = option::side_formula{}(init_v);
        common::get<option::devices>(init_v) = option::device_formula{}(init_v);
        // Construct the network object.
        net_t network{init_v};
        // Run the simulation until exit.
        network.run();
    }
    // Builds the resulting plots.
    std::cout << "*/\n" << plot::file("graphic", plotter.build());
    return 0;
}
