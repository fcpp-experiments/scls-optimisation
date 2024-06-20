// Copyright © 2024 Giorgio Audrito. All Rights Reserved.

/**
 * @file setup.hpp
 * @brief Network configuration of the experimental evaluation.
 */

#include "lib/fcpp.hpp"
#include "lib/somewhere.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief The simulation time after which a device becomes true.
constexpr size_t true_time = 100;
//! @brief The simulation time after which that device becomes false again.
constexpr size_t false_time = 2*true_time;
//! @brief The final simulation time.
constexpr size_t end_time = 3*true_time;
//! @brief Communication radius.
constexpr size_t comm = 100;
//! @brief Dimensionality of the space.
constexpr size_t dim = 2;
//! @brief Height of the deployment area.
constexpr size_t height = comm;


//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {


//! @brief Tags used in the node storage.
namespace tags {
    //! @brief The variance of round timing in the network.
    struct tvar {};
    //! @brief The number of hops in the network.
    struct hops {};
    //! @brief The density of devices.
    struct dens {};
    //! @brief The movement speed of devices.
    struct speed {};
    //! @brief The number of devices.
    struct devices {};
    //! @brief The side of deployment area.
    struct side {};
    //! @brief Color of the current node.
    struct node_color {};
    //! @brief Size of the current node.
    struct node_size {};
    //! @brief Shape of the current node.
    struct node_shape {};

    //! @brief Oracle somewhere implementation.
    struct oracle {};
    //! @brief Ideal somewhere implementation.
    struct baseline {};
    //! @brief Ideal somewhere implementation.
    struct kfree {};
    //! @brief Ideal somewhere implementation.
    struct replicated {};
    //! @brief Ideal somewhere implementation.
    struct fastest {};

    //! @brief The truth value computed by an implementation.
    template <typename T> struct value {};
    //! @brief The error of an implementation.
    template <typename T> struct error {};
    //! @brief The size of messages used by an implementation.
    template <typename T> struct msg_size {};
}


//! @brief Executes a somewhere implementation and stores data about it in the node storage.
GEN(F, ...Ts) void reporter(ARGS, F&& fun, Ts&&... xs) { CODE
    using namespace tags;

    size_t msg_base = node.cur_msg_size();
    node.storage(value<F>{}) = fun(CALL, std::forward<Ts>(xs)...);
    node.storage(msg_size<F>{}) = node.cur_msg_size() - msg_base;
    node.storage(error<F>{}) = node.storage(value<F>{}) != node.storage(value<somewhere::oracle>{});
}
//! @brief Export types used by the reporter function.
GEN_EXPORT(F) reporter_t = export_list<typename F::export_t>;
//! @brief Storage tags and types used by the reporter function.
GEN_EXPORT(F) reporter_s = storage_list<
    tags::value<F>,         bool,
    tags::error<F>,         bool,
    tags::msg_size<F>,      size_t
>;

//! @brief Main function.
MAIN() {
    using namespace tags;
    hops_t diameter = node.net.storage(hops{});
    real_t infospeed = 70;
    int replicas = 3;

    // random walk into a given rectangle with given speed
    rectangle_walk(CALL, make_vec(0,0), make_vec(1,1)*node.net.storage(side{}), node.net.storage(speed{}), 1);

    // the value of the formula for the current event
    bool somewhere_f = node.current_time() > true_time and node.current_time() < false_time;
    bool formula = node.uid == 0 and somewhere_f;

    reporter(CALL, somewhere::oracle{}, formula, somewhere_f);
    reporter(CALL, somewhere::baseline{}, formula, diameter);
    reporter(CALL, somewhere::knowledge_free{}, formula);
    reporter(CALL, somewhere::replicated{}, formula, diameter, infospeed, replicas);
    reporter(CALL, somewhere::fastest{}, formula, diameter, infospeed);

    // usage of node storage
    node.storage(node_size{}) = formula ? 20 : 10;
    node.storage(node_color{}) = node.storage(value<somewhere::replicated>{}) ? color(RED) : color(GREEN);
    node.storage(node_shape{}) = node.storage(value<somewhere::baseline>{}) ? shape::star : shape::sphere;
}
//! @brief Export types used by the main function.
FUN_EXPORT main_t = export_list<
    rectangle_walk_t<2>, 
    reporter_t<somewhere::oracle>,
    reporter_t<somewhere::baseline>,
    reporter_t<somewhere::knowledge_free>,
    reporter_t<somewhere::replicated>,
    reporter_t<somewhere::fastest>
>;
//! @brief Storage tags and types used by the main function.
FUN_EXPORT main_s = storage_list<
    reporter_s<somewhere::oracle>,
    reporter_s<somewhere::baseline>,
    reporter_s<somewhere::knowledge_free>,
    reporter_s<somewhere::replicated>,
    reporter_s<somewhere::fastest>,
    tags::node_color,           color,
    tags::node_shape,           shape,
    tags::node_size,            double
>;

} // namespace coordination


//! @brief Namespace for component options.
namespace option {

//! @brief Import tags to be used for component options.
using namespace component::tags;
//! @brief Import tags used by aggregate functions.
using namespace coordination::tags;


//! @brief The randomised sequence of rounds for every node (about one every second, with 10% variance).
using round_s = sequence::periodic<
    distribution::interval_n<times_t, 0, 1>, // uniform time in the [0,1] interval for start
    distribution::weibull< // weibull-distributed time for interval (mean 1, deviation equal to tvar divided by 100)
        distribution::constant_n<double, 1>,
        functor::div<distribution::constant_i<double, tvar>, distribution::constant_n<double, 100>>
    >,
    distribution::constant_n<times_t, end_time+2>  // the constant end_time+2 number for end
>;
//! @brief The sequence of network snapshots (one every simulated second).
using log_s = sequence::periodic_n<1, 0, 1, end_time>;
//! @brief The sequence of node generation events (multiple devices all generated at time 0).
using spawn_s = sequence::multiple<
    distribution::constant_i<size_t, devices>,
    distribution::constant_n<double, 0>
>;
//! @brief The distribution of initial node positions (random in a square).
using rectangle_d = distribution::rect<
    distribution::constant_n<double, 0>,
    distribution::constant_n<double, 0>,
    distribution::constant_i<double, side>,
    distribution::constant_i<double, side>
>;

//! @brief The tags and corresponding aggregators to be logged (change as needed).
template <typename T>
using algorithm_aggr = storage_list<
    value<T>,          aggregator::mean<double>,
    error<T>,          aggregator::mean<double>,
    msg_size<T>,       aggregator::mean<double>
>;
using aggregator_t = storage_list<
    node_size, aggregator::mean<double>,
    algorithm_aggr<coordination::somewhere::oracle>,
    algorithm_aggr<coordination::somewhere::baseline>,
    algorithm_aggr<coordination::somewhere::knowledge_free>,
    algorithm_aggr<coordination::somewhere::replicated>,
    algorithm_aggr<coordination::somewhere::fastest>
>;
//! @brief The aggregator to be used on logging rows for plotting.
using row_aggregator_t = common::type_sequence<aggregator::mean<double>>;
//! @brief The logged values to be shown in plots as lines given unit U.
template <template<class> class U>
using points_t = plot::values< aggregator_t, row_aggregator_t, plot::unit<U> >;
//! @brief A generic plot given unit U, split tag S and filters Fs.
template <template<class> class U, typename S, typename... Fs>
using gen_plot_t = plot::split<
    S,
    plot::filter< Fs..., points_t<U> >
>;
//! @brief A generic row of plots given split tag S and filters Fs.
template <typename S, typename... Fs>
using plot_row_t = plot::split<common::type_sequence<>, plot::join<
    gen_plot_t<value,S,Fs...>,
    gen_plot_t<error,S,Fs...>,
    gen_plot_t<msg_size,S,Fs...>
>>;
//! @brief A plot of the logged values by time for tvar,dens,hops,speed = 10 (default values).
using time_plot_t = plot_row_t<plot::time, tvar, filter::equal<10>, dens, filter::equal<10>, hops, filter::equal<10>, speed, filter::equal<10>>;
//! @brief A plot of the logged values by tvar for times >= true_time (after the first formula switch).
using tvar_plot_t = plot_row_t<tvar, plot::time, filter::above<true_time>, dens, filter::equal<10>, hops, filter::equal<10>, speed, filter::equal<10>>;
//! @brief A plot of the logged values by dens for times >= true_time (after the first formula switch).
using dens_plot_t = plot_row_t<dens, plot::time, filter::above<true_time>, tvar, filter::equal<10>, hops, filter::equal<10>, speed, filter::equal<10>>;
//! @brief A plot of the logged values by hops for times >= true_time (after the first formula switch).
using hops_plot_t = plot_row_t<hops, plot::time, filter::above<true_time>, tvar, filter::equal<10>, dens, filter::equal<10>, speed, filter::equal<10>>;
//! @brief A plot of the logged values by speed for times >= true_time (after the first formula switch).
using speed_plot_t = plot_row_t<speed, plot::time, filter::above<true_time>, tvar, filter::equal<10>, dens, filter::equal<10>, hops, filter::equal<10>>;
//! @brief Combining the plots into a single row.
using plot_t = plot::join<time_plot_t, tvar_plot_t, dens_plot_t, hops_plot_t, speed_plot_t>;

// computes side length from hops
struct side_formula {
    template <typename T>
    size_t operator()(T const& x) const {
        double h = common::get<hops>(x);
        return h * comm / sqrt(2.0) + 0.5;
    }
};
// computes device number from dens and side
struct device_formula {
    template <typename T>
    size_t operator()(T const& x) const {
        double d = common::get<dens>(x);
        double s = common::get<side>(x);
        return d*s*s/(3.141592653589793*comm*comm) + 0.5;
    }
};

//! @brief The general simulation options.
DECLARE_OPTIONS(list,
    parallel<false>,     // no multithreading on node rounds
    synchronised<false>, // optimise for asynchronous networks
    program<coordination::main>,   // program to be run (refers to MAIN above)
    exports<coordination::main_t>, // export type list (types used in messages)
    round_schedule<round_s>, // the sequence generator for round events on nodes
    retain<metric::retain<3,1>>,   // messages are kept for 3 seconds before expiring
    log_schedule<log_s>,     // the sequence generator for log events on the network
    spawn_schedule<spawn_s>, // the sequence generator of node creation events on the network
    node_store<    // the contents of the node storage
        coordination::main_s
    >,
    net_store<     // the contents of the net storage
        side,       double,
        hops,       double,
        speed,      double
    >,
    aggregators<aggregator_t>,  // the tags and corresponding aggregators to be logged
    init<
        x,          rectangle_d // initialise position randomly in a rectangle for new nodes
    >,
    // general parameters to use for plotting
    extra_info<
        tvar,   double,
        dens,   double,
        hops,   double,
        speed,  double
    >,
    plot_type<plot_t>, // the plot description to be used
    dimension<dim>, // dimensionality of the space
    connector<connect::fixed<comm, 1, dim>>, // connection allowed within a fixed comm range
    shape_tag<node_shape>, // the shape of a node is read from this tag in the store
    size_tag<node_size>,   // the size of a node is read from this tag in the store
    color_tag<node_color> // colors of a node are read from these
);

} // namespace option

} // namespace fcpp
