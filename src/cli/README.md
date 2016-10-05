# dim-cli

Classes for making unix style command line interfaces.

Main features:
- parses directly to variables (or makes proxies for them)
- supports parsing to any type that is:
  - is default constructable
  - assignable from std::string or has an istream extraction operator
- help page generation
- composable subcommands (Just kidding!)

What does it look like?
```C++
int main(int argc, char ** argv) {
    Cli cli;
    auto & count = cli.arg<int>("c count", 1).desc("times to say hello");
    auto & name = cli.arg<string>("name", "Unknown").desc("who to greet");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (!name)
        cout << "Using the unknown name." << endl;
    for (unsigned i = 0; i < *count; ++i)
        cout << "Hello " << *name << "!" << endl;
    return EX_OK;
}
```

What it looks like when run:
```console
$ a.out --count=3  
Using the unknown name.  
Hello Unknown!  
Hello Unknown!  
Hello Unknown!  
```

It automatically generates nicely formatted help pages:
```console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --help               Show this message and exit.
  -c, --count INTEGER  times to say hello
  --name STRING        who to greet
```

## Basic Usage
After inspecting args the Cli parser returns false if it thinks the program 
should exit. Cli::exitCode() will be set to either EX_OK (because of an early 
exit like --help) or EX_USAGE for bad arguments.

```C++
int main(int argc, char ** argv) {
    Cli cli;
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the apple have a worm? No!";
    return EX_OK;
}
```

And what it looks like:
```console
$ a.out
Does the apple have a worm? No!
$ a.out --help  
Usage: a.out [OPTIONS]  

Options:  
  --help    Show this message and exit.  
```

## Arguments
Cli is used by declaring targets to receive arguments. Either via pointer
to a predefined variable or by implicitly creating the variable as part of the 
call declaring the target.

Use arg<T>(names, defaultValue) to link a target variable with argument 
names. It returns a proxy object that can be used like a pointer (* and ->) 
to access the target directly.

```C++
int main(int argc, char ** argv) {
    Cli cli;
    auto & fruit = cli.arg<string>("fruit", "apple");
    if (!cli.parse(cerr, argc, argv)) {
        return cli.exitCode();
    }
    cout << "Does the " << *fruit << " have a worm? No!";
    return EX_OK;
}
```

And what it looks like:
```console
$ a.out --fruit=orange
Does the orange have a worm? No!
$ a.out --help  
Usage: a.out [OPTIONS]  

Options:  
  --fruit STRING  
  --help          Show this message and exit.  
```

## Argument Names
Names are passed in as a space separated list of names:
  
- f - single character (short name) named argument
- file - multicharacter (long name) named argument
- [file name] - optional positional argument
- <file> - required positional argument

Names for positionals (inside brackets) may contain spaces, and all names may 
be preceded by modifiers:

- ! - for boolean values, when setting the value it is first inverted
- ? - for non-boolean named arguments, makes the value optional (the name can 
appear without the value)

Long names for boolean values always get a second "no-" version implicitly
created.

For example:
```C++
int main(int argc, char ** argv) {
    Cli cli;
    cli.arg<string>("f ?foo [foo]").desc("foo the bar");
    cli.arg<bool>("!b bar").desc("bar the baz");
    cli.arg<string>("<baz>").desc("all the baz");
    cli.parse(cerr, argc, argv);
    return EX_OK;
}
```
Ends up looking like this (note: required positionals always go before any 
optional ones):
```console
$ a.out --help  
Usage: a.out [OPTIONS] <baz> [foo]  
  baz       all the baz  
  foo       foo the bar  

Options:  
  -f, --foo STRING  foo the bar  
  -b, --[no-]bar    bar the baz  
```

When named arguments are added they replace any previous rule with the same 
name, therefore these args declare '-n' an inverted bool:
```C++
cli.arg<bool>("n !n");
```
But now '-n' is a string:
```C++
cli.arg<bool>("n !n");
cli.arg<string>("n");
```

## Positional Arguments
- required are placed before optional
- only one can have nargs = -1
  - if the -1 nargs arg is required, it prevents optionals from matching
  - if it's optional, it prevents subsequent optionals from matching


## Boolean Arguments
Long names for boolean values always get a second "no-" version implicitly
created.

- feature, using boolValue()

## Vector Arguments
- nargs = -1 for unlimited

## Special Arguments
- "-"
- "--"

## External Variables
- external targets
- binding targets multiple times
  - rebinding to internal proxy targets

```C++
int main(int argc, char ** argv) {
    bool worm;
    Cli cli;
    cli.arg(&worm, "w worm").desc("make it icky");
    auto & fruit = cli.arg<string>("fruit", "apple").desc("type of fruit");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the " << *fruit << " have a worm? " 
        << worm ? "Yes :(" : "No!";
    return EX_OK;
}
```
And what it looks like:
```console
$ a.out --fruit=orange
Does the orange have a worm? No!
$ a.out --help  
Usage: a.out [OPTIONS]  

Options:  
  --fruit STRING  type of fruit
  --help          Show this message and exit.  
```

## Life After Parsing
- dereferencing proxies
- testing if proxy was triggered
- getting the name that triggered a proxy

## Variable Modifiers
- desc
- implicitValue, using with named arguments with optional values
- count, -vvv -> 3, must be integral? convertable from integral? :P
- boolValue(bool isDefault), "feature switch"
- choice(vector<T>&), value from vector, index in vec for enum?, 
or vector<pair<string, T>>?
- prompt(string&, bool hide, bool confirm)
- argPassword
- yes, are you sure?
- range
- clamp

## How to keep Cli::parse() from doing IO
For some applications, such as Windows services, it's important not to 
interact with the console. There some simple steps to stop parse() from doing 
any console IO:

1. Don't use options (such as Arg<T>::prompt()) that explicitly it to do IO.
2. Add your own "help" argument to override the default, you can still 
turn around and call Cli::writeHelp(ostream&) if desired.
3. Use the two argument version of Cli::parse() so it doesn't output errors 
immediately. The text of any error that may have occurred during a parse is 
available via Cli::errMsg()

## Aspirational Roadmap
- callbacks
- help text and formatting options
- environment variables?
- support /name:value syntax
- composable subcommands
- unrecognized named options?
- tuple arguments
