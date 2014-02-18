// Copyright (C) 2014  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_CROSS_VALIDATE_TRACK_ASSOCIATION_TrAINER_H__
#define DLIB_CROSS_VALIDATE_TRACK_ASSOCIATION_TrAINER_H__

#include "cross_validate_track_association_trainer_abstract.h"
#include "structural_track_association_trainer.h"

namespace dlib
{
// ----------------------------------------------------------------------------------------

    namespace impl
    {
        template <
            typename track_association_function,
            typename detection_type,
            typename detection_id_type
            >
        void test_track_association_function (
            const track_association_function& assoc,
            const std::vector<std::vector<std::pair<detection_type,detection_id_type> > >& samples,
            unsigned long& total_dets,
            unsigned long& correctly_associated_dets
        )
        {
            const typename track_association_function::association_function_type& f = assoc.get_assignment_function();

            typedef typename detection_type::track_type track_type;
            using namespace impl;

            std::vector<track_type> tracks;
            std::map<detection_id_type,unsigned long> track_idx; // tracks[track_idx[id]] == track with ID id.

            for (unsigned long j = 0; j < samples.size(); ++j)
            {
                total_dets += samples[j].size();
                std::vector<long> assignments = f(get_unlabeled_dets(samples[j]), tracks);
                std::vector<bool> updated_track(tracks.size(), false);
                // now update all the tracks with the detections that associated to them.
                const std::vector<std::pair<detection_type,detection_id_type> >& dets = samples[j];
                for (unsigned long k = 0; k < assignments.size(); ++k)
                {
                    if (assignments[k] != -1)
                    {
                        tracks[assignments[k]].update_track(dets[k].first);
                        updated_track[assignments[k]] = true;

                        // if this detection was supposed to go to this track
                        if (track_idx.count(dets[k].second) && track_idx[dets[k].second]==assignments[k])
                            ++correctly_associated_dets;

                        track_idx[dets[k].second] = assignments[k];
                    }
                    else
                    {
                        track_type new_track;
                        new_track.update_track(dets[k].first);
                        tracks.push_back(new_track);

                        // if this detection was supposed to go to a new track
                        if (track_idx.count(dets[k].second) == 0)
                            ++correctly_associated_dets;

                        track_idx[dets[k].second] = tracks.size()-1;
                    }
                }

                // Now propagate all the tracks that didn't get any detections.
                for (unsigned long k = 0; k < updated_track.size(); ++k)
                {
                    if (!updated_track[k])
                        tracks[k].propagate_track();
                }
            }
        }
    }

// ----------------------------------------------------------------------------------------

    template <
        typename track_association_function,
        typename detection_type,
        typename detection_id_type
        >
    double test_track_association_function (
        const track_association_function& assoc,
        const std::vector<std::vector<std::vector<std::pair<detection_type,detection_id_type> > > >& samples
    )
    {
        unsigned long total_dets = 0;
        unsigned long correctly_associated_dets = 0;

        for (unsigned long i = 0; i < samples.size(); ++i)
        {
            impl::test_track_association_function(assoc, samples[i], total_dets, correctly_associated_dets);
        }

        return (double)correctly_associated_dets/(double)total_dets;
    }

// ----------------------------------------------------------------------------------------

    template <
        typename trainer_type,
        typename detection_type,
        typename detection_id_type
        >
    double cross_validate_track_association_trainer (
        const trainer_type& trainer,
        const std::vector<std::vector<std::vector<std::pair<detection_type,detection_id_type> > > >& samples,
        const long folds
    )
    {
        const long num_in_test  = samples.size()/folds;
        const long num_in_train = samples.size() - num_in_test;

        std::vector<std::vector<std::vector<std::pair<detection_type,detection_id_type> > > > samples_train;

        long next_test_idx = 0;
        unsigned long total_dets = 0;
        unsigned long correctly_associated_dets = 0;

        for (long i = 0; i < folds; ++i)
        {
            samples_train.clear();

            // load up the training samples
            long next = (next_test_idx + num_in_test)%samples.size();
            for (long cnt = 0; cnt < num_in_train; ++cnt)
            {
                samples_train.push_back(samples[next]);
                next = (next + 1)%samples.size();
            }

            const typename trainer_type::trained_function_type& df = trainer.train(samples_train);
            for (long cnt = 0; cnt < num_in_test; ++cnt)
            {
                impl::test_track_association_function(df, samples[next_test_idx], total_dets, correctly_associated_dets);
                next_test_idx = (next_test_idx + 1)%samples.size();
            }
        }

        return (double)correctly_associated_dets/(double)total_dets;
    }

// ----------------------------------------------------------------------------------------

}

#endif // DLIB_CROSS_VALIDATE_TRACK_ASSOCIATION_TrAINER_H__


