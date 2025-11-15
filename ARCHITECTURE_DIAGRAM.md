# ExpoCLI Architecture and Code Flow Diagrams

## 1. Overall Query Execution Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                     USER INPUT (Query String)                    │
│                   "SELECT name FROM ./files"                     │
└───────────────────────┬─────────────────────────────────────────┘
                        │
                        ▼
        ┌───────────────────────────────────────┐
        │        LEXER (lexer.cpp)              │
        │   Tokenizes input string              │
        │   Returns: Token[]                    │
        └───────────────┬───────────────────────┘
                        │
                        ▼
        ┌───────────────────────────────────────┐
        │        PARSER (parser.cpp)            │
        │   Parses tokens → AST                 │
        │   Returns: Query struct               │
        └───────────────┬───────────────────────┘
                        │
                        ▼
        ┌───────────────────────────────────────────────────┐
        │        QUERY EXECUTOR (query_executor.cpp)       │
        │                                                   │
        │  1. getXmlFiles() - Find files                   │
        │  2. shouldUseThreading() - Decision              │
        │  3. executeMultithreaded() OR loop              │
        │  4. processFile() - Process each file            │
        │  5. Apply ORDER BY, LIMIT, DISTINCT             │
        └───────────────┬───────────────────────────────────┘
                        │
                        ▼
        ┌───────────────────────────────────────────────────┐
        │      XML NAVIGATOR (xml_navigator.cpp)           │
        │     [Called from within processFile()]           │
        │                                                   │
        │  - extractValues() - Get field values            │
        │  - evaluateWhereExpr() - Filter rows             │
        │  - findNodesByPartialPath() - Tree traversal     │
        └───────────────┬───────────────────────────────────┘
                        │
                        ▼
        ┌───────────────────────────────────────┐
        │   RESULT FORMATTER (result_formatter) │
        │   Formats output for display          │
        └───────────────┬───────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────────┐
│                    OUTPUT (Formatted Results)                    │
│                     name                                         │
│                     ------                                       │
│                     Alice                                        │
│                     Bob                                          │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Multi-Threading Execution Flow

```
┌──────────────────────────────────────┐
│     executeMultithreaded()           │
│     (files: [f0, f1, ... f99])      │
└────────────┬─────────────────────────┘
             │
             ▼
    ┌────────────────────────────┐
    │ Create N worker threads    │
    │ N = getOptimalThreadCount()│
    │ (usually 4-16)             │
    └────────┬───────────────────┘
             │
    ┌────────┴────────┬─────────────┬──────────────┐
    │                 │             │              │
    ▼                 ▼             ▼              ▼
┌────────────┐ ┌────────────┐ ┌──────────┐ ┌──────────┐
│ Thread 0   │ │ Thread 1   │ │Thread 2  │ │Thread 3  │
│            │ │            │ │          │ │          │
│ Process:   │ │ Process:   │ │Process:  │ │Process:  │
│ f0,f4,f8.. │ │ f1,f5,f9.. │ │f2,f6,f10 │ │f3,f7,f11 │
│            │ │            │ │          │ │          │
│ Lock       │ │ Lock       │ │Lock      │ │Lock      │
│ resultMtx  │ │ resultMtx  │ │resultMtx │ │resultMtx │
│ Append     │ │ Append     │ │Append    │ │Append    │
│ results    │ │ results    │ │results   │ │results   │
│            │ │            │ │          │ │          │
│ Increment  │ │ Increment  │ │Increment │ │Increment │
│ completed  │ │ completed  │ │completed │ │completed │
└────┬───────┘ └────┬───────┘ └──┬───────┘ └──┬───────┘
     │              │             │           │
     └──────────────┼─────────────┼───────────┘
                    │             │
                    ▼             ▼
            ┌────────────────────────────────┐
            │   Combine results vector      │
            │   (All threads wait here)     │
            │   thread.join()               │
            └────────────┬───────────────────┘
                         │
                         ▼
            ┌────────────────────────────────┐
            │  Return merged results        │
            │  (Single vector with all data)│
            └────────────────────────────────┘
```

---

## 3. XML Processing Per File

```
┌─────────────────────────────────────────┐
│  processFile(filepath, query)           │
│  (One per XML file)                    │
└────────────┬────────────────────────────┘
             │
             ▼
  ┌────────────────────────┐
  │  XmlLoader::load()     │
  │  Load file to memory   │
  │  DOM tree creation     │
  └────────────┬───────────┘
               │
               ▼ (pugi::xml_document)
  ┌────────────────────────────────┐
  │  Check for FOR clauses         │
  │  or WHERE conditions           │
  └────────────┬───────────────────┘
               │
        ┌──────┴──────┐
        │             │
  Has FOR Clauses?    No FOR Clauses?
   (Advanced)         (Standard)
        │             │
        ▼             ▼
    ┌─────────┐   ┌─────────────────────┐
    │Process  │   │For each field in    │
    │Nested   │   │SELECT:              │
    │FOR      │   │                     │
    │loops    │   │ extractValues()     │
    │         │   │                     │
    │Context  │   └──────┬──────────────┘
    │binding  │          │
    └────┬────┘          ▼
         │      ┌──────────────────────┐
         │      │ For each node in     │
         │      │ XML tree:            │
         │      │ evaluateWhere()      │
         │      │ Filter matching rows │
         │      └──────┬───────────────┘
         │             │
         └─────┬───────┘
               │
               ▼
    ┌─────────────────────────┐
    │  ResultRow vector       │
    │  (Matching records)     │
    └─────────────────────────┘
```

---

## 4. Performance-Critical: findNodesByPartialPath()

```
┌────────────────────────────────────────────────────┐
│  findNodesByPartialPath(rootNode, pathComponents)  │
│  [CRITICAL BOTTLENECK - Called per query per file] │
└───────────────┬────────────────────────────────────┘
                │
                ▼
    ┌───────────────────────────────────┐
    │  Recursive searchTree() lambda    │
    │  (Traverses entire XML tree)      │
    └───────────────┬───────────────────┘
                    │
        ┌───────────┴───────────┐
        │                       │
        ▼ (For each node)       │
    ┌─────────────────────┐    │
    │ getNodePath()       │    │
    │                     │    │ Recursive
    │ Walk to root and    │    │ call on
    │ build path vector   │    │ children
    │ (EXPENSIVE!)        │    │
    └─────────┬───────────┘    │
              │                │
              ▼                │
    ┌─────────────────────┐    │
    │ endsWithPath()      │    │
    │                     │    │
    │ Check if node path  │    │
    │ matches target      │    │
    └─────────┬───────────┘    │
              │                │
            Match?             │
              │ Yes            │
              ▼                │
    ┌──────────────────┐       │
    │ Add to results   │       │
    │ vector           │       │
    └──────────────────┘       │
                               │
        ┌──────────────────────┘
        │
        ▼ (For each child)
    ┌─────────────────┐
    │ Recurse to      │
    │ children        │
    │ (Back to start) │
    └─────────────────┘

COMPLEXITY ANALYSIS:
- Tree has N nodes
- For each node: getNodePath() = O(D) where D=depth
- Total: O(N*D) traversals + O(N*D) path building = O(N*D²)
- Called multiple times per query!
```

---

## 5. WHERE Clause Evaluation Flow

```
┌──────────────────────────────────┐
│  evaluateWhereExpr(node, WHERE)  │
│  (Checks if node matches WHERE)  │
└────────────┬─────────────────────┘
             │
        ┌────┴────┐
        │         │
        ▼         ▼
  ┌─────────┐  ┌──────────┐
  │Condition│  │Logical   │
  │(Simple) │  │(AND/OR)  │
  └────┬────┘  └────┬─────┘
       │             │
       ▼             ▼
  ┌─────────────┐ ┌─────────────┐
  │Eval single  │ │Recursively  │
  │comparison   │ │eval left &  │
  │             │ │right, then  │
  │getNodeValue()│ │combine with │
  │             │ │AND/OR       │
  └────┬────────┘ └────┬────────┘
       │                │
       ├────────────────┘
       │
       ▼
    ┌──────────────────────────┐
    │  getNodeValue()          │
    │  [MODERATE BOTTLENECK]   │
    │                          │
    │  For multi-component:    │
    │  calls findNodesByPartial│
    │  Path() - EXPENSIVE!     │
    └────────┬─────────────────┘
             │
             ▼
    ┌──────────────────────┐
    │  compareValues()     │
    │  String/numeric      │
    │  comparison          │
    └──────────────────────┘
```

---

## 6. Result Processing Pipeline

```
Input: Raw ResultRow vector
       ResultRow = vector<pair<fieldName, fieldValue>>

       │
       ▼
┌─────────────────────────┐
│  Apply DISTINCT (opt)   │
│                         │
│  For each row:          │
│   Build "row key"       │
│   Check if seen before  │
│   O(R log R) complexity │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│  Apply ORDER BY (opt)   │
│                         │
│  std::sort() with       │
│  custom comparator      │
│                         │
│  Linear field lookup    │
│  per comparison:        │
│  O(F*R*log R)           │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│  Apply OFFSET (opt)     │
│  Erase first N rows     │
│  O(R)                   │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│  Apply LIMIT (opt)      │
│  Resize to N rows       │
│  O(1) with resize       │
└──────────┬──────────────┘
           │
           ▼
Output: Final ResultRow vector
        Ready for display
```

---

## 7. File-Level Parallelism vs Within-File Parallelism

### Current Implementation: FILE-LEVEL (Good for Multi-file)

```
Files: [f0, f1, f2, f3, f4, ... f99]

Thread Pool (4 threads):
┌────────────────────────────────┐
│ Thread 0: Process f0 ──→ f4 ──→ f8  │
│ Thread 1: Process f1 ──→ f5 ──→ f9  │
│ Thread 2: Process f2 ──→ f6 ──→ f10 │
│ Thread 3: Process f3 ──→ f7 ──→ f11 │
└────────────────────────────────┘

Pros:
- Load balanced: each thread gets ~N/T files
- Minimal synchronization (just result merge)
- Works well when files are independent

Cons:
- Can't parallelize within single large file
- Synchronous: slowest thread blocks others
- Limited speedup for single-file queries
```

### Potential: WITHIN-FILE Parallelism (For optimization)

```
Single Large File:
┌──────────────────────────────────┐
│  Root Node                       │
│  ├─ Section 1 ─ Thread 0         │
│  ├─ Section 2 ─ Thread 1         │
│  ├─ Section 3 ─ Thread 2         │
│  └─ Section 4 ─ Thread 3         │
└──────────────────────────────────┘

Would require:
- Partitioning large XML documents
- Merging partial results
- More complex WHERE evaluation
- Better for nodes >> files scenario
```

---

## 8. Data Structure Analysis

### Current: Vector<Pair> for Results

```cpp
ResultRow = std::vector<std::pair<std::string, std::string>>
             ^         ^field name   ^field value

Pros:
- Preserves column order
- Simple iteration

Cons:
- Linear lookup O(F) per field access
- Used in ORDER BY: O(F*R*log R) total
- String key comparisons (not hashed)
```

### Better for ORDER BY: Indexed Access

```cpp
// Could use:
struct ResultRowIndexed {
    std::vector<std::string> values;
    std::unordered_map<std::string, size_t> fieldIndex;
    // Fast lookup: O(1) instead of O(F)
};
```

---

## 9. Memory Usage Pattern

```
┌─────────────────────────────────────────┐
│  Memory Timeline During Multi-File Query │
└─────────────────────────────────────────┘

Single-Threaded:
┌────────────────────────────────────┐
│ File 1: Loaded → Processed → Freed │
│ File 2: Loaded → Processed → Freed │
│ File 3: Loaded → Processed → Freed │
└────────────────────────────────────┘
Max Memory: ~1 × file_size

Multi-Threaded (N threads):
┌──────────────────────────────────────────────┐
│ File 1: Loaded (T0) ──┐                      │
│ File 2: Loaded (T1) ──┼─ Simultaneous       │
│ File 3: Loaded (T2) ──┤   processing        │
│ File 4: Loaded (T3) ──┘                     │
└──────────────────────────────────────────────┘
Max Memory: ~N × file_size
(Up to 16 × file_size with current setup)

Result Accumulation:
┌─────────────────────────────────┐
│ Results vector grows over time  │
│ Lock/unlock overhead on each    │
│ thread result insertion         │
└─────────────────────────────────┘
```

---

## 10. Bottleneck Interaction Map

```
                    Query String
                         │
                    ┌────▼─────┐
                    │ Lexer (✓) │  Fast: O(Q)
                    └────┬─────┘
                         │
                    ┌────▼─────┐
                    │Parser (✓) │  Fast: O(Q)
                    └────┬─────┘
                         │
        ┌────────────────▼────────────────┐
        │  Query Executor (Main Bottleneck)
        │                                 │
        │  ├─ File Discovery         ✓ Ok
        │  │
        │  ├─ Multi-threading        ✓ Good
        │  │
        │  └─ processFile() × N      ⚠ SLOW!
        │         │
        │         ├─ XML Load         ⚠ Memory Heavy
        │         │
        │         └─ extractValues()  ⚠⚠⚠ CRITICAL!
        │            │
        │            └─ findNodesByPartialPath()
        │               │
        │               ├─ getNodePath()     ⚠⚠ EXPENSIVE
        │               │  (Called N*D times)
        │               │
        │               └─ endsWithPath()    ✓ Fast O(M)
        │
        │         └─ WHERE Evaluation ⚠⚠ REPEATED!
        │            │
        │            └─ getNodeValue()
        │               (Calls findNodesByPartialPath again!)
        │
        │  ├─ Result Merge           ⚠ Mutex Contention
        │  │
        │  ├─ DISTINCT              ⚠ String Key Build
        │  │
        │  ├─ ORDER BY              ⚠⚠ Linear Field Lookup
        │  │
        │  └─ LIMIT/OFFSET          ✓ Fast

Legend: ✓ = Not a bottleneck
        ⚠ = Moderate concern
        ⚠⚠ = Significant bottleneck
        ⚠⚠⚠ = Critical bottleneck
```

