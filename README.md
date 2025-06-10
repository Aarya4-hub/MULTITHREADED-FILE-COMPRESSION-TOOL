# MULTITHREADED-FILE-COMPRESSION-TOOL

COMPANY NAME -CODTECH IT SOLUTIONS

NAME -AARYA DIPAK POUL

INTERN ID -CT08DK522

DOMAIN NAME -C++ PROGRAMMING

DURATION -8 WEEKS(APRIL 20th to JUNE 20th 2025)

MENTOR -NEELA SANTHOSH KUMAR



📍Overview


This Multithreaded File Compression and Decompression Tool is a high-performance application built in C++ that compresses and decompresses files using the Zlib compression library, enhanced with multithreading for faster execution. The tool splits large files into smaller chunks, processes each chunk concurrently in multiple threads, and then writes the compressed output in order. This model simulates a modern, real-world file compression utility optimized for performance on multi-core systems.

The project demonstrates practical applications of data compression algorithms, thread synchronization, I/O optimization, and parallel programming. It is an ideal project for intermediate to advanced programmers interested in systems programming, file handling, and multithreaded architectures.



🛠️How It Works


The application operates in two modes: compression and decompression, selectable by the user.

Compression Mode:
Initialization:

The input file (input.txt) is opened in binary mode.

The file is divided into fixed-size chunks (1 MB each).

Each chunk is assigned a unique index to preserve ordering.

Chunk Compression:

Each chunk is processed in a separate thread using the compress2() function from Zlib.

The chunk is compressed with maximum compression level (Z_BEST_COMPRESSION).

Once compressed, the chunk is pushed into a shared std::queue.

Thread Synchronization:

A std::mutex and std::condition_variable ensure safe access to the shared queue.

A writer thread waits for compressed chunks and writes them to the output file (output.z) in order.

Metadata Handling:

Each chunk’s index and size are saved alongside its compressed data.

This metadata ensures that decompression restores the original file structure accurately.

Decompression Mode:
Initialization:

The compressed file (output.z) is read in chunks.

Each chunk is prefixed with its index and size, allowing the tool to extract the compressed segment.

Multithreaded Decompression:

Each compressed chunk is processed in a separate thread using the uncompress() function from Zlib.

Threads push the decompressed data into a queue.

Output Writing:

A writer thread ensures decompressed chunks are written to the output file (decompressed.txt) in the correct order using a buffer map to handle out-of-order chunks.

Resource Cleanup:

Threads are joined after processing, and files are closed properly.



📌Features


Multithreaded Execution: Utilizes multiple CPU cores to compress/decompress large files faster.

Chunk-Based Processing: Splits files into manageable parts for better memory efficiency and parallelism.

Zlib Integration: Uses the industry-standard compression library for reliable and efficient compression.

Order-Preserving Output: Maintains chunk order using indexing and out-of-order buffering.

Error Handling: Detects file I/O issues and compression errors (e.g., decompression failure).

Metadata Storage: Saves chunk index and size during compression for accurate decompression.

Scalable Design: Easily adapts to larger files or different compression algorithms.


📚Learning Objectives


This project teaches core concepts in systems programming, multithreading, and file compression:

Multithreading and Concurrency:
Understand how to launch, manage, and synchronize multiple threads using C++ STL (std::thread, std::mutex, std::condition_variable).

Learn how to prevent race conditions and maintain data consistency in concurrent environments.

Data Compression:
Gain experience using the Zlib library for compressing and decompressing binary data.

Learn how compression algorithms work and how to apply them chunk-wise.

File I/O and Buffering:
Work with binary file input/output streams.

Learn techniques for buffering data to and from files efficiently.

Chunk-Based Processing:
Understand the benefits of dividing a large task into smaller units for parallel processing.

Handle edge cases such as partial reads and end-of-file behavior.

Queue and Buffer Management:
Use standard data structures like std::queue and std::map for managing data across threads.

Implement producer-consumer patterns using condition variables.

Indexing and Ordering:
Learn how to maintain the correct sequence of data chunks when multithreaded operations complete in an unpredictable order.

Implement mechanisms to store and reorder data using indices.

Error Detection and Reporting:
Handle failures like unreadable files or failed decompression gracefully.

Learn how to use return codes from external libraries (like Zlib) for debugging and control flow.

