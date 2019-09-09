//*****************************************************************************
// Copyright 2017-2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#include "ngraph/op/softmax.hpp"

#include <algorithm>

#include "ngraph/builder/autobroadcast.hpp"
#include "ngraph/op/constant.hpp"
#include "ngraph/op/multiply.hpp"
#include "ngraph/op/reshape.hpp"
#include "ngraph/op/subtract.hpp"
#include "ngraph/op/sum.hpp"
#include "ngraph/util.hpp"

using namespace std;
using namespace ngraph;

// *** SOFTMAX OP SET 0 ***
const string op::Softmax::type_name{"Softmax"};

std::shared_ptr<op::Constant> AxisSetToConstant(const AxisSet& axes)
{
    vector<int64_t> v;
    for (auto axis : axes)
    {
        v.push_back(axis);
    }
    return op::Constant::create(element::i64, Shape{axes.size()}, v);
}

op::v0::Softmax::Softmax(const Output<Node>& arg, const AxisSet& axes)
    : Op({arg, AxisSetToConstant(axes)->output(0)})
{
    constructor_validate_and_infer_types();
}

op::v0::Softmax::Softmax(const Output<Node>& arg, const Output<Node>& axes)
    : Op({arg, axes})
{
    constructor_validate_and_infer_types();
}

AxisSet op::v0::Softmax::get_axes() const
{
    auto const_op = dynamic_pointer_cast<op::Constant>(input_value(1).get_node_shared_ptr());
    if (const_op)
    {
        return AxisSet(const_op->get_vector<size_t>());
    }
    else
    {
        ngraph_error("get_axes called on a Softmax node whose 'axes' input is not constant");
    }
}

void op::v0::Softmax::set_axes(const AxisSet& axes)
{
    this->input(1).replace_source_output(AxisSetToConstant(axes)->output(0));
}

void op::v0::Softmax::validate_and_infer_types()
{
    const PartialShape& input_shape = get_input_partial_shape(0);
    auto m_axes = get_axes();
    NODE_VALIDATION_CHECK(this,
                          input_shape.rank().is_static(),
                          "Input node rank must be static (input_shape=",
                          input_shape,
                          ").");
    for (auto axis : m_axes)
    {
        NODE_VALIDATION_CHECK(this,
                              axis >= 0 && axis < static_cast<size_t>(input_shape.rank()),
                              "Reduction axis (",
                              axis,
                              ") is out of bounds (argument shape: ",
                              input_shape,
                              ").");
    }
    if (input_shape.is_static())
    {
        set_output_type(0, get_input_element_type(0), input_shape.to_shape());
    }
    else
    {
        set_output_type(0, get_input_element_type(0), PartialShape::dynamic());
    }

    // empty axes == all axes
    if (m_axes.size() == 0)
    {
        for (size_t i = 0; i < get_shape().size(); ++i)
        {
            m_axes.insert(i);
        }
    }
}

shared_ptr<Node> op::v0::Softmax::copy_with_new_args(const NodeVector& new_args) const
{
    check_new_args_count(this, new_args);
    auto m_axes = get_axes();
    return make_shared<Softmax>(new_args.at(0), m_axes);
}

void op::v0::Softmax::generate_adjoints(autodiff::Adjoints& adjoints, const NodeVector& deltas)
{
    auto delta = deltas.at(0);
    auto m_axes = get_axes();

    auto z = delta * shared_from_this();
    auto zsum = make_shared<op::Sum>(z, m_axes);

    Shape shape;
    for (size_t i = 0; i < get_shape().size(); ++i)
    {
        if (m_axes.find(i) == m_axes.end())
        {
            shape.push_back(get_shape()[i]);
        }
        else
        {
            shape.push_back(1);
        }
    }
    auto order = ngraph::get_default_order(zsum->get_shape());
    auto zreshape = make_shared<op::Reshape>(zsum, order, shape);

    auto adjoint = z - builder::make_with_numpy_broadcast<op::Multiply>(output(0), zreshape);

    auto x = input_value(0);
    adjoints.add_delta(x, adjoint);
}

// *** SOFTMAX OP SET V1 ***
const string op::v1::Softmax::type_name{"Softmax"};

op::v1::Softmax::Softmax(const Output<Node>& arg, const size_t axis)
    : Op({arg})
    , m_axis(axis)
{
    constructor_validate_and_infer_types();

    const PartialShape& input_shape = get_input_partial_shape(0);
    NODE_VALIDATION_CHECK(this,
                          input_shape.rank().is_static(),
                          "Input node rank must be static (input_shape=",
                          input_shape,
                          ").");
    NODE_VALIDATION_CHECK(this,
                          axis >= 0 && axis < static_cast<size_t>(input_shape.rank()),
                          "Reduction axis (",
                          axis,
                          ") is out of bounds (argument shape: ",
                          input_shape,
                          ").");

    if (input_shape.is_static())
        set_output_type(0, get_input_element_type(0), input_shape.to_shape());
    else
        set_output_type(0, get_input_element_type(0), PartialShape::dynamic());
}

shared_ptr<Node> op::v1::Softmax::copy_with_new_args(const NodeVector& new_args) const
{
    check_new_args_count(this, new_args);
    return make_shared<op::v1::Softmax>(new_args.at(0), m_axis);
}

void op::v1::Softmax::generate_adjoints(autodiff::Adjoints& adjoints, const NodeVector& deltas)
{
    throw ngraph_error("op::v1::Softmax::generate_adjoints function is not implemented yet");

    /* This might work, but as of this writing we have no way to test it, so we are being careful
    auto delta = deltas.at(0);

    auto z = delta * shared_from_this();

    std::vector<size_t> axes(get_shape().size() - m_axis);
    std::iota(std::begin(axes), std::end(axes), m_axis);
    AxisSet axes_set{axes};

    auto zsum = make_shared<op::Sum>(z, axes_set);

    Shape shape;
    for (size_t i = 0; i < get_shape().size(); ++i)
    {
        if (axes_set.find(i) == axes_set.end())
        {
            shape.push_back(get_shape()[i]);
        }
        else
        {
            shape.push_back(1);
        }
    }
    auto order = ngraph::get_default_order(zsum->get_shape());
    auto zreshape = make_shared<op::Reshape>(zsum, order, shape);

    auto adjoint = z - builder::make_with_numpy_broadcast<op::Multiply>(output(0), zreshape);

    auto x = input(0).get_source_output();
    adjoints.add_delta(x, adjoint);
    */
}
