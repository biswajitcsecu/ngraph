#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ngraph/ngraph.hpp"
#include "ngraph/pass/manager.hpp"
#include "ngraph/pass/opset0_downgrade.hpp"
#include "ngraph/pass/opset1_upgrade.hpp"
#include "util/type_prop.hpp"

using namespace std;
using namespace ngraph;

TEST(opset_transform, opset1_ont_hot_downgrade_pass)
{
    auto indices = make_shared<op::Parameter>(element::i64, Shape{1, 3, 2, 3});
    auto depth = op::Constant::create(element::i64, Shape{}, {4});
    auto on_value = op::Constant::create(element::u32, Shape{}, {5});
    auto off_value = op::Constant::create(element::u32, Shape{}, {10});
    int64_t axis = 3;
    auto ont_hot_v1 = make_shared<op::v1::OneHot>(indices, depth, on_value, off_value, axis);

    auto result = make_shared<op::Result>(ont_hot_v1);
    auto f = make_shared<Function>(ResultVector{result}, ParameterVector{indices});

    ngraph::pass::Manager pass_manager;
    pass_manager.register_pass<pass::Opset0Downgrade>();
    pass_manager.run_passes(f);

    const auto pass_replacement_node =
        f->get_result()->input(0).get_source_output().get_node_shared_ptr();
    const auto one_hot_v0 = static_pointer_cast<op::v0::OneHot>(pass_replacement_node);

    EXPECT_EQ(one_hot_v0->get_shape(), (Shape{1, 3, 2, 4, 3}));
}

TEST(opset_transform, opset1_ont_hot_downgrade_pass_depth_not_constant)
{
    auto indices = make_shared<op::Parameter>(element::i64, Shape{1, 3, 2, 3});
    auto depth = make_shared<op::Parameter>(element::i64, Shape{});
    auto on_value = op::Constant::create(element::u32, Shape{}, {5});
    auto off_value = op::Constant::create(element::u32, Shape{}, {10});
    int64_t axis = 3;
    auto ont_hot_v1 = make_shared<op::v1::OneHot>(indices, depth, on_value, off_value, axis);

    auto result = make_shared<op::Result>(ont_hot_v1);
    auto f = make_shared<Function>(ResultVector{result}, ParameterVector{indices, depth});

    ngraph::pass::Manager pass_manager;
    pass_manager.register_pass<pass::Opset0Downgrade>();

    try
    {
        pass_manager.run_passes(f);
        // Should have thrown, so fail if it didn't
        FAIL() << "Not constant depth not detected";
    }
    catch (const ngraph_error& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(), std::string("depth input must be constant"));
    }
    catch (...)
    {
        FAIL() << "Not constant depth not detected for unexpected reason";
    }
}

TEST(opset_transform, opset1_ont_hot_downgrade_pass_indices_shape_not_static)
{
    auto indices = make_shared<op::Parameter>(element::i64, PartialShape::dynamic());
    auto depth = op::Constant::create(element::i64, Shape{}, {4});
    auto on_value = op::Constant::create(element::u32, Shape{}, {5});
    auto off_value = op::Constant::create(element::u32, Shape{}, {10});
    int64_t axis = 3;
    auto ont_hot_v1 = make_shared<op::v1::OneHot>(indices, depth, on_value, off_value, axis);

    auto result = make_shared<op::Result>(ont_hot_v1);
    auto f = make_shared<Function>(ResultVector{result}, ParameterVector{indices});

    ngraph::pass::Manager pass_manager;
    pass_manager.register_pass<pass::Opset0Downgrade>();

    try
    {
        pass_manager.run_passes(f);
        // Should have thrown, so fail if it didn't
        FAIL() << "Not static indices shape not detected";
    }
    catch (const ngraph_error& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(), std::string("indices shape must be static"));
    }
    catch (...)
    {
        FAIL() << "Not static indices shape not detected for unexpected reason";
    }
}
