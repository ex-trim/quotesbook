# Quotesbook

Quotesbook is a command-line utility which displays a random quotation from a collection of quotes.

You can:
  - Input and save quotes
  - Get random quote of a day
  - Get quote by number

## Sample usage

    $ quotesbook
    Your work is going to fill a large part of your life, and the only way to be truly satisfied 
    is to do what you believe is great work. And the only way to do great work is to love what 
    you do. If you havent found it yet, keep looking. Dont settle. As with all matters of the heart, 
    you will know when you find it. [ Steve Jobs ]
    $

### History
I'm looking for something like fortune (BSD/UNIX programm), but a want to be able to add to the collection my own quotes and manage them. That's why i 
prefer to write this program.

### Version
0.1

### Installation

Qoutesbook requires [sqlite3 lib](https://www.sqlite.org/download.html) to be installed to compile with.

Installation steps are simple:

```sh
$ cd ./quotes
$ make
$ make install
```

### Development

Want to contribute? Great!

Make a change and pull it in a new branch.

### Todos

 - Return struct instead of printf in db_query

License
----

MIT

**Free Software, Hell Yeah!**
