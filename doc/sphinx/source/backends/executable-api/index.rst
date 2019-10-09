.. backends/executable-api/index.rst:


Executable
==========

As of version ``0.15``, there is a new backend API to work with functions that 
can be compiled as a runtime ``Executable``. Where previously ``Backend`` used a 
``shared_ptr<Function>`` as the handle passed to the ``call`` method to execute 
a compiled object, the addition of the ``shared_ptr<Executable>`` object has 
more direct methods to actions such as ``validate``, ``call``, ``get_performance_data``, 
and so on. This new API permits any executable to be saved or loaded *into* or 
*out of* storage and makes it easier to distinguish when a Function is compiled,
thus making the internals of the ``Backend`` and ``Executable`` easier to 
implement.  


The ``compile`` function on an ``Executable`` has more direct methods to 
actions such as ``validate``, ``call``, ``get_performance_data``, and so on. 

.. doxygenclass:: ngraph::runtime::Executable
   :project: ngraph
   :members: 

