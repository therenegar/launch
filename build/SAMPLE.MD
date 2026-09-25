# Launch! Markdown Sample

This document is both a test file for !MKDOWN and a compact syntax reference.
It intentionally exercises the Markdown features understood by Launch!.

## Contents

- [Paragraphs and inline styles](#paragraphs-and-inline-styles)
- [Heading levels](#heading-levels)
- [Block quotes](#block-quotes)
- [Lists](#unordered-lists)
- [Tables](#pipe-table)
- [Links](#links)

Use these internal links in **Show** to test heading navigation.

## Paragraphs and inline styles

This is a normal paragraph of dummy text. Lorem ipsum dolor sit amet,
consectetur adipiscing elit. Integer vel neque sit amet lorem malesuada
fringilla. Suspendisse potenti. Curabitur vitae justo at erat luctus posuere.

Plain text can contain *italic text*, _italic text_, **bold text**,
***bold italic text***, and `inline code`.

Launch! also supports ~~strikethrough~~ and <del>HTML-style strike</del>.
Underline can be written as <u>HTML underline</u> or with the Launch!
extension {{double-brace underline}}.

Superscript uses HTML tags, for example E = mc<sup>2</sup>.
Subscript works too, for example H<sub>2</sub>O.

Styles can be combined: ***bold italic***, **bold with `code` nearby**,
and {{underlined **bold** text}}.

Escaped Markdown characters can be shown literally: \*stars\* and \# hash.

## Heading levels

# Heading 1
## Heading 2
### Heading 3
#### Heading 4
##### Heading 5
###### Heading 6

## Block quotes

> This is a block quote. It should be visually distinct from ordinary body
> text and is useful for quotations, notes, and callouts.

> A second quoted paragraph provides enough text to test wrapping and
> continuation across the preview width.

## Unordered lists

- First item
- Second item
  - Nested item A
  - Nested item B
- Third item

* Asterisk list marker
+ Plus list marker

## Ordered lists

1. First numbered item
2. Second numbered item
3. Third numbered item
   1. Nested numbered item
   2. Another nested item

## Task lists

[ ] An unfinished task
[x] A completed task
[X] Completed with an upper-case X

## Definition list

Launch!
: A text-mode launcher and accessory suite for DOS.

Markdown
: A lightweight markup language used for formatted plain-text documents.

## Horizontal rules

---

Text between horizontal rules.

***

## Links

This is an [inline link](https://example.com/launch).

Launch! also recognizes an automatic URL such as
<https://example.com/documentation>.

## Pipe table

| Feature | Status | Notes |
|---------|--------|-------|
| Headings | Yes | Levels 1 through 6 |
| Lists | Yes | Ordered and unordered |
| Tables | Yes | Rendered with box drawing |
| Code | Yes | Inline and fenced |
| Quotes | Yes | Styled block quotes |

## Plain-column table

Name          Type          Example
------------  ------------  ----------------
Alpha         Text          Ordinary content
Beta          Number        12345
Gamma         Mixed         DOS / VGA / EGA

## Fenced code block

```c
#include <stdio.h>

int main(void)
{
    printf("Hello from DOS!\n");
    return 0;
}
```

~~~text
A tilde fence is supported too.
This block should be rendered in the code style.
~~~

## Indented code

    REM This is an indented code block
    ECHO Launch! Markdown sample
    DIR C:\DOCS

## Images and media placeholders

Images are not rendered as raster graphics, but their Markdown syntax is
recognized and represented by a media placeholder:

![Example image](example.bmp "Example image")

Raw HTML media tags are also recognized as placeholders:

<img src="portrait.bmp" alt="Portrait">
<video src="demo.avi" title="Demo video"></video>
<audio src="sample.wav" title="Sample audio"></audio>
<iframe src="example.html" title="Embedded page"></iframe>

## Footnote-style markers

Launch! recognizes footnote-style markers in graphical output.[^1]

[^1]: This line is useful for testing the small footnote-style rendering.

## Long wrapping paragraph

Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque sed
diam vitae lacus suscipit aliquam. Morbi feugiat, sem at placerat faucibus,
erat justo condimentum velit, non tempor magna lectus vitae metus. Praesent
fermentum, mauris vitae feugiat malesuada, libero erat sollicitudin sapien,
vitae tincidunt justo magna sit amet ligula. This paragraph is deliberately
long enough to exercise soft wrapping in the editor, text preview, Focus
mode, maximized view, graphical Show mode, printing, and exported output.

## Character and punctuation test

ASCII punctuation:
! " # $ % & ' ( ) * + , - . / : ; < = > ? @ [ \ ] ^ _ ` { | } ~

Numbers:
0123456789

Common DOS text:
C:\LAUNCH\!MKDOWN.EXE SAMPLE.MD

---

End of Launch! Markdown sample.
