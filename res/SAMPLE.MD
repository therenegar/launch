# An h1 header

Paragraphs are separated by a blank line.

2nd paragraph. *Italic*, **bold**, {{underline}} and `monospace`.
That underline is custom for Launch! because underline is useful in DOS.
Itemized lists look like:

  * this one
  * that one
  * the other one

Note that --- not considering the asterisk --- the actual text
content starts at 4-columns in.

> Block quotes are
> written like so.
>
> They can span multiple paragraphs,
> if you like.

## An h2 header

Here's a numbered list:

 1. first item
 2. second item
 3. third item

Note again how the actual text starts at 4 columns in (4 characters
from the left side). Here's a code sample:
```
    # Let me re-iterate ...
    for i in 1 .. 10 { do-something(i) }
```
As you probably guessed, indented 4 spaces. By the way, instead of
indenting the block, you can use delimited blocks, if you like:

```
define foobar() {
    print "Welcome to flavor country!";
}
```

(which makes copying & pasting easier). Syntax highlighting will come one day, 
and when it does it will use this to determine code type:

```python
import time
# Quick, count to ten!
for i in range(10):
    # (but not *too* quick)
    time.sleep(0.5)
    print i
```
### An h3 header

Now a nested list:

 1. First, get these ingredients:

      - carrots
      - celery
      - lentils

 2. Boil some water.

 3. Dump everything in the pot and follow
    this algorithm:

        1. find wooden spoon
        2. uncover pot
        3. stir
        4. cover pot
        5. balance wooden spoon precariously on pot handle
        6. wait 10 minutes
        7. goto first step (or shut off burner when done)

> Do not bump wooden spoon or it will fall!

Notice again how text always lines up on 4-space indents (including
that last line which continues item 3 above).

Here's a link to [a website](http://foo.bar), to a [local
doc](local-doc.html), and to a [section heading in the current
doc](#an-h2-header). 

Here's a footnote [^1].

#### A level 4 Heading

Tables can look like this:

| Tables   |      Are      |  Cool |
|----------|:-------------:|------:|
| col 1 is |  left-aligned | $1600 |
| col 2 is |    centered   |   $12 |
| col 3 is | right-aligned |    $1 |


| heading                   | heading | heading                               | heading | this heading  wraps |
|---------------------------|---------|---------------------------------------|---------|---------------------|
| This is a multi line cell | x       | x                                     |         | $                   |
|                           |         | This cell wraps  over multiple  lines |         | $                   |
|                           |         |                                       |         | $                   |

A horizontal rule follows.

---

##### Finally H5

We're getting to the end of our sample document.

Here's a definition list:

apples
  : Good for making applesauce.
oranges
  : Citrus!
tomatoes
  : There's no "e" in tomatoe.

Again, text is indented 4 spaces. (Put a blank line between each
term/definition pair to spread things out more.)



[^1]: Footnote text goes here.






















































































































































































































































































































