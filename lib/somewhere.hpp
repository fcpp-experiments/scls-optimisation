// Copyright © 2024 Giorgio Audrito. All Rights Reserved.

/**
 * @file somewhere.hpp
 * @brief Alternative implementations of the somewhere operator.
 */

#ifndef FCPP_SOMEWHERE_H_
#define FCPP_SOMEWHERE_H_

#include "lib/coordination/election.hpp"
#include "lib/coordination/past_ctl.hpp"
#include "lib/coordination/slcs.hpp"
#include "lib/coordination/time.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

/**
 * Generic algorithm replicator, returning the value of the oldest
 * replica currently running.
 *
 * @param fun The aggregate code to replicate (without arguments).
 * @param n   The number of replicas.
 * @param t   The interval between replica spawning.
 */
GEN(F) auto replicate(ARGS, F fun, size_t n, times_t t) { CODE
    size_t now = shared_clock(CALL) / t;
    auto res = spawn(CALL, [&](size_t i){
        return make_tuple(fun(), i > now - n);
    }, common::option<size_t, true>{now});
    for (auto x : res) if (x.first > now - n) now = min(now, x.first);
    return res[now];
}
//! @brief Export list for replicate.
FUN_EXPORT replicate_t = export_list<spawn_t<size_t, bool>, shared_clock_t>;


//! @brief Grouping different somewhere implementations.
namespace somewhere {

//! @brief Oracle implementation.
struct oracle {
    FUN bool operator()(ARGS, bool, bool val) const { CODE
        return val;
    }
    FUN_EXPORT export_t = export_list<>;
};

//! @brief State-of-the-art baseline implementation.
struct baseline {
    FUN bool operator()(ARGS, bool f, hops_t diameter) const { CODE
        return abf_hops(CALL, f) < diameter;
    }
    FUN_EXPORT export_t = export_list<abf_hops_t>;
};

//! @brief Knowledge-free implementation.
struct knowledge_free {
    FUN bool operator()(ARGS, bool f) const { CODE
        return not get<0>(wave_election(CALL, make_tuple(not f, node.uid)));
    }
    FUN_EXPORT export_t = export_list<wave_election_t<tuple<bool,device_t>>>;
};

//! @brief Implementation by replicating the EP past-CTL operator.
struct replicated {
    FUN bool operator()(ARGS, bool f, hops_t diameter, real_t infospeed, size_t replicas) const { CODE
        return replicate(CALL, [&](){
            return logic::EP(CALL, f);
        }, replicas, diameter / infospeed / (replicas-1));
    }
    FUN_EXPORT export_t = export_list<replicate_t, past_ctl_t>;
};

//! @brief Models a view of a data for all devices of a network.
struct netstate {
    //! @brief Default constructor.
    netstate() : data(make_tuple(-INF, false)) {}

    //! @brief Initialising constructor.
    netstate(field<tuple<times_t, bool>> data) : data(data) {}

    //! @brief Updates the data stored for a single device.
    void update(device_t id, times_t time, bool val) {
        fcpp::details::self(data, id) = make_tuple(time,val);
    }

    //! @brief Checks whether there is a true stored for a device with a timestamp after the threshold.
    bool value(times_t threshold) const {
        for (auto const& t : fcpp::details::get_vals(data))
            if (get<0>(t) > threshold and get<1>(t))
                return true;
        return false;
    }

    //! @brief Calculates the pointwise maximum of two netstates.
    static netstate max(netstate const& x, netstate const& y) {
        return fcpp::max(x.data, y.data);
    }

    //! @brief Serialises the content from/to a given input/output stream.
    template <typename S>
    S& serialize(S& s) {
        return s & data;
    }

    //! @brief Serialises the content from/to a given input/output stream (const overload).
    template <typename S>
    S& serialize(S& s) const {
        return s << data;
    }

    //! @brief The actual data, stored as a field of tuples.
    field<tuple<times_t, bool>> data;
};

//! @brief Fastest and heaviest implementation.
struct fastest {
    FUN bool operator()(ARGS, bool f, hops_t diameter, real_t infospeed) const { CODE
        return nbr(CALL, netstate{}, [&](field<netstate> n){
            netstate s = fold_hood(CALL, netstate::max, n);
            s.update(node.uid, node.current_time(), f);
            return make_tuple(s.value(node.current_time() - diameter / infospeed), std::move(s));
        });
    }
    FUN_EXPORT export_t = export_list<netstate>;
};

} // namespace somewhere

} // namespace coordination

} // namespace fcpp

#endif // FCPP_SOMEWHERE_H_
