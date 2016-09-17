# qlog

An extremely quick templated logging framework focused on a specific use case of critical path logging. Gurantees performance equal to copy for the caller.

    - Header only.
    - Both synchronous and asynchronous logging. However, synchronous logging is basically a templated wrapper over fprintf/fstream.
    - One would want to use it when the performance of the caller thread is extremely critical, even so that string conversion should also be offloaded to a different thread.
    - Best used for csv (or other delimiter) style single line logging.
    - Supports compile time strings. See `StringCT`

## Getting Started
- Add the `include` folder in your include path.
- Use `LoggerManager<>` to declare the appropriate logger. Check examples.

### Prerequisities
- gcc 4.8.3 or later.
- google benchmark for running benchmark code.

```
Give examples
```
## Running the tests
[TODO]

### Break down into end to end tests
```
Give an example [TODO]
```

### And coding style tests

```
Give an example [TODO]
```

## Contributing

[TODO]

## Versioning

[TODO]

## Authors

* **Dhanjit Das** - *Initial work* - [dhanjit](https://github.com/dhanjit)

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

[TODO] 

