// Copyright (c) 2021 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef DISTANCESCORINGCRITERION_H
#define DISTANCESCORINGCRITERION_H

#include "geometry/Point2LL.h"
#include "utils/scoring/ScoringCriterion.h"

namespace cura
{
class PointsSet;

/*!
 * Criterion that will give a score according to the distance from the point to a target point. Closer points will get
 * a higher score.
 */
class DistanceScoringCriterion : public ScoringCriterion
{
private:
    const PointsSet& points_;
    const Point2LL& target_pos_;

public:
    explicit DistanceScoringCriterion(const PointsSet& points, const Point2LL& target_pos);

    virtual double computeScore(const size_t candidate_index) const override;
};

} // namespace cura

#endif // DISTANCESCORINGCRITERION_H
