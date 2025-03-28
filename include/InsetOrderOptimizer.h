// Copyright (c) 2024 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher

#ifndef INSET_ORDER_OPTIMIZER_H
#define INSET_ORDER_OPTIMIZER_H

#include <unordered_set>

#include "settings/ZSeamConfig.h"
#include "sliceDataStorage.h"

namespace cura
{

class FffGcodeWriter;
class LayerPlan;

class InsetOrderOptimizer
{
public:
    using value_type = std::unordered_multimap<const ExtrusionLine*, const ExtrusionLine*>;

    /*!
     * Constructor for inset ordering optimizer.
     *
     * This constructor gets basically all of the locals passed when it needs to
     * optimise the order of insets.
     * \param gcode_writer The FffGcodeWriter on whose behalf the inset order is
     * being optimized.
     * \param storage Read slice data from this storage.
     * \param gcode_layer The layer where the resulting insets must be planned.
     * \param mesh The mesh that these insets are part of.
     * \param extruder_nr Which extruder to process. If an inset is not printed
     * with this extruder, it will not be added to the plan.
     * \param mesh_config The path configs for a single mesh, indicating the
     * line widths, flows, speeds, etc to print this mesh with.
     * \param part The part from which to read the previously generated insets.
     * \param layer_nr The current layer number.
     */
    InsetOrderOptimizer(
        const FffGcodeWriter& gcode_writer,
        const SliceDataStorage& storage,
        LayerPlan& gcode_layer,
        const Settings& settings,
        const int extruder_nr,
        const GCodePathConfig& inset_0_default_config,
        const GCodePathConfig& inset_X_default_config,
        const GCodePathConfig& inset_0_roofing_config,
        const GCodePathConfig& inset_X_roofing_config,
        const GCodePathConfig& inset_0_flooring_config,
        const GCodePathConfig& inset_X_flooring_config,
        const GCodePathConfig& inset_0_bridge_config,
        const GCodePathConfig& inset_X_bridge_config,
        const bool retract_before_outer_wall,
        const coord_t wall_0_wipe_dist,
        const coord_t wall_x_wipe_dist,
        const size_t wall_0_extruder_nr,
        const size_t wall_x_extruder_nr,
        const ZSeamConfig& z_seam_config,
        const std::vector<VariableWidthLines>& paths,
        const Point2LL& model_center_point,
        const Shape& disallowed_areas_for_seams = {},
        const bool scarf_seam = false,
        const bool smooth_speed = false,
        const Shape& overhang_areas = Shape());

    /*!
     * Adds the insets to the given layer plan.
     *
     * The insets and the layer plan are passed to the constructor of this
     * class, so this optimize function needs no additional information.
     * \return Whether anything was added to the layer plan.
     */
    bool addToLayer();

    /*!
     * Get the order constraints of the insets when printing walls per region / hole.
     * Each returned pair consists of adjacent wall lines where the left has an inset_idx one lower than the right.
     *
     * Odd walls should always go after their enclosing wall polygons.
     *
     * \param outer_to_inner Whether the wall polygons with a lower inset_idx should go before those with a higher one.
     */
    static value_type getRegionOrder(const std::vector<ExtrusionLine>& input, const bool outer_to_inner);

    /*!
     * Get the order constraints of the insets when printing walls per inset.
     * Each returned pair consists of adjacent wall lines where the left has an inset_idx one lower than the right.
     *
     * Odd walls should always go after their enclosing wall polygons.
     *
     * \param outer_to_inner Whether the wall polygons with a lower inset_idx should go before those with a higher one.
     */
    static value_type getInsetOrder(const auto& input, const bool outer_to_inner);

private:
    const FffGcodeWriter& gcode_writer_;
    const SliceDataStorage& storage_;
    LayerPlan& gcode_layer_;
    const Settings& settings_;
    const size_t extruder_nr_;
    const GCodePathConfig& inset_0_default_config_;
    const GCodePathConfig& inset_X_default_config_;
    const GCodePathConfig& inset_0_roofing_config_;
    const GCodePathConfig& inset_X_roofing_config_;
    const GCodePathConfig& inset_0_flooring_config_;
    const GCodePathConfig& inset_X_flooring_config_;
    const GCodePathConfig& inset_0_bridge_config_;
    const GCodePathConfig& inset_X_bridge_config_;
    const bool retract_before_outer_wall_;
    const coord_t wall_0_wipe_dist_;
    const coord_t wall_x_wipe_dist_;
    const size_t wall_0_extruder_nr_;
    const size_t wall_x_extruder_nr_;
    const ZSeamConfig& z_seam_config_;
    const std::vector<VariableWidthLines>& paths_;
    const LayerIndex layer_nr_;
    const Point2LL model_center_point_; // Center of the model (= all meshes) axis-aligned bounding-box.
    Shape disallowed_areas_for_seams_;
    const bool scarf_seam_;
    const bool smooth_speed_;
    Shape overhang_areas_;

    std::vector<std::vector<const Polygon*>> inset_polys_; // vector of vectors holding the inset polygons
    Shape retraction_region_; // After printing an outer wall, move into this region so that retractions do not leave visible blobs. Calculated lazily if needed (see
                              // retraction_region_calculated).

    /*!
     * Given a closed polygon, insert a seam point at the point where the seam should be placed.
     * This should result in the seam-finding algorithm finding that exact point, instead of the
     * 'best' vertex on that polygon. Under certain circumstances, the seam-placing algorithm can
     * however still deviate from this, for example when the seam-point placed here isn't suppored
     * by the layer below.
     *
     * \param closed_line The polygon to insert the seam point in. (It's assumed to be closed at least.)
     *
     * \return The index of the inserted seam point, or the index of the closest point if an existing one can be used.
     */
    std::optional<size_t> insertSeamPoint(ExtrusionLine& closed_line);

    /*!
     * Determine if the paths should be reversed
     * If there is one extruder used, and we're currently printing the inner walls then Reversing the insets now depends on the inverse of
     * the inset direction. If we want to print the outer insets first we start with the lowest and move forward otherwise we start with the
     * highest and iterate back.
     * Otherwise, if the wall is partially printed with the current extruder we need to move forward for the outer wall extruder and iterate
     * back for the inner wall extruder
     *
     * \param use_one_extruder boolean stating that we are using a single extruder.
     * \param current_extruder_is_wall_x boolean stating if the current extruder is used for the inner walls.
     * \param outer_to_inner boolean which should be set to true if we're printing from an outside to the inside
     *
     * \return a bool if the paths should be printed in reverse or not
     */
    constexpr bool shouldReversePath(const bool use_one_extruder, const bool current_extruder_is_wall_x, const bool outer_to_inner);

    /*!
     * Flattens the `paths` and sorts the walls that should be printed added depending on if it is a single outer wall or the inner wall(s),
     * The order can be reversed if required.
     *
     * \param reverse boolean stating if the order of the wall should be revered or not
     * \param use_one_extruder lean stating that we are using a single extruder.
     *
     * \return A vector of ExtrusionLines with walls that should be printed
     */
    std::vector<ExtrusionLine> getWallsToBeAdded(const bool reverse, const bool use_one_extruder);
};
} // namespace cura

#endif // INSET_ORDER_OPTIMIZER_H
