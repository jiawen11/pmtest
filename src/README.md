## Implementation of trace verification

In the simplest case, the generated trace would contain 5 operations:
* 3 logging operations that are automatically generated by NVMVeri:
	* `Assign(&A, sizeof(A))`: The value of A is modified by a value assignment, so the address range of A is touched.
	* `Flush(&A, sizeof(A))`:	The value of A is flushed into memory, so the address range of A is persisted.
	* `Fence()`: The fence operation which ensures that the operations happen before and after this are not reordered.
* 2 query operations that are inserted by user. By inserting operations at certain positions, the user will know:
	* `Persist(&A, sizeof(A))`: Is the address range of A is persisted in memory?
	* `Order(&A, sizeof(A), &B, sizeof(B))`: Is the address range of A touched strictly earlier than address range of B, e.g. they are seperated by a fence?

For example, for an integer A, the address range of A is an interval like `[0x7ffe523ac210, 0x7ffe523ac214)`, but for an user defined structure, the address range may be larger. To identify the order of modification, we update the *timestamp* for each address after each modification.

Because the overhead of storing each address and corresponding timestamp is not acceptable, to support `Persist` and `Order` queries, we need a data structure that efficiently stores *intervals*, and the related *timestamp* of intervals.

We adopt a data structure named **interval tree**, which takes up O(n \* log n) space and supports the following operations in O(log n) time:
* Add an interval;
* Remove an interval;
* Given an interval x, find if x overlaps with any of the existing intervals.

This data structure is implemented in boost library.

We maintain 2 interval trees:
* A *persistence-check* tree, that stores all address ranges that are modified but not yet flushed to memory. So we
	* Add address interval A to it after `Assign(&A, sizeof(A))`;
	* Remove address interval A from it after `Flush(&A, sizeof(A))`.
* An *order-check* tree, that stores address ranges and the latest time it is modified, represented by the *timestamp*. So we
	* Maintain a global variable timestamp, and initialize it to 0;
	* Increase the timestamp by one after `Fence()`;
	* Add address interval A and current timestamp after `Assign(&A, sizeof(A))`. If there exists some overlap between A and the existing intervals in this tree, update the timestamp of the overlapped part and add the remaining part that is not overlapped to the tree.
	
### Examples of interval tree:
* Add address `[18, 34)` to the *persistence-check* tree:
```
Existing |---------|    |--------------|  |-----|
         10        20   25             40 43    49
		 
Add              |---------------|
                 18              34
				 
Result   |-----------------------------|  |-----|
         10                            40 43    49
```

* Add address `[18, 34)` with timestamp 2 to the *order-check* tree:
```
             t=0              t=1           t=1
Existing |---------|    |--------------|  |-----|
         10        20   25             40 43    49

                        t=2
Add              |---------------|
                 18              34
	
            t=0         t=2        t=1      t=1 	
Result   |-------|---------------|-----|  |-----|
         10      18              34    40 43    49
```
### Examples of transactions and queries:
```
Assign(&A, 4)
Fence()
Assign(&B, 4)
Flush(&A, 4)
Persist(&A, 4)		// == true
Order(&A, 4, &B, 4)	// == true
```
```
Assign(&A, 4)
Fence()
Assign(&B, 4)
Assign(&A, 4)
Flush(&A, 4)
Fence()
Order(&A, 4, &B, 4)	// == false
```
```
Assign(&A, 4)
Assign(&B, 4)
Fence()
Assign(&B, 4)
Persist(&B, 4)		// == false
Order(&A, 4, &B, 4)	// == true
```
```
Assign(&A, 4)
Fence()
Assign(&B, 4)
Flush(&A, 4)
Assign(&A, 4)
Persist(&A, 4)		// == false
Order(&A, 4, &B, 4)	// == false
```