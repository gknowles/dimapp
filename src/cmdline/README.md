# cmdline

Classes for making unix style command line interfaces.

Main features:
* parses directly to variables (or makes proxies for them)
* supports parsing to any type that is:
** assignable from std::string
** has an istream extraction operator

What does it look like?
```C++
int main(int argc, char ** argv) {
    CmdParser cli;
    auto & count = cli.arg<int>("count", 1);
    auto & name = cli.arg<string>("name", "Unknown");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitcode();
    if (!name)
        cout << "Using the unknown name." << endl;
    for (unsigned i = 0; i < *count; ++i)
        cout << "Hello " << *name << "!" << endl;
    return EX_OK;
}
```

What it looks like when run:
```
>hello --count=3
Using the unknown name.
Hello Unknown!
Hello Unknown!
Hello Unknown!
```

It automatically generates nicely formatted help pages:
```
Just kidding, it doesn't do this at all!
```

## Basic Concepts
Cmdline is used by declaring targets to receive arguments. Either via pointer
to a predefined variable or by implicitly creating the variable as part of the 
call declaring the target.

Use arg(...) to link a target variable with argument names. It returns a proxy 
object that can be used like a pointer (* and ->) to access the target directly.

```C++
int main(int argc, char ** argv) {
    CmdParser cli;
    auto & fruit = cli.arg<string>("fruit", "apple").desc("type of fruit");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitcode();
    cout << "Does the " << *fruit << " have a worm? No!";
    return EX_OK;
}
```

And what it looks like:
> $ a.out --fruit=orange
> Does the orange have a worm? No!
> $ a.out --help  
> Usage: a.out [OPTIONS]  
>
> Options:  
>   --fruit TEXT  type of fruit
>   --help        Show this message and exit.  



```C++
int main(int argc, char ** argv) {
    bool worm;
    CmdParser cli;
    cli.arg(&worm, "w worm").desc("make it icky");
    auto & fruit = cli.arg<string>("fruit", "apple").desc("type of fruit");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitcode();
    cout << "Does the " << *fruit << " have a worm? " 
        << worm ? "Yes :(" : "No!";
    return EX_OK;
}
```
