import {
  ServalMarkdownView,
  ServalMarkdownStyle,
  ServalMarkdownStyleSheet,
  ServalMarkdownConfig,
} from "@lynx/serval_markdown";
import { createNode } from "./Button";

interface MarkdownStyleExtended extends ServalMarkdownStyleSheet {
  ".red"?: ServalMarkdownStyle,
  ".bold"?: ServalMarkdownStyle,
  ".big"?: ServalMarkdownStyle,
  ".small"?: ServalMarkdownStyle,
  ".underline"?: ServalMarkdownStyle,
  unorderedListMarker ?: ServalMarkdownStyle,
  table ?: ServalMarkdownStyle,
  tableCell?: ServalMarkdownStyle,
}

@Component
export struct Markdown {
  private servalMarkdownView = new ServalMarkdownView()
  private servalMarkdownStyle: MarkdownStyleExtended = {
    h1:{
      marginTop: 50,
    },
    quote: {
      color: "808080"
    },
    link: {
      color: "0000ff"
    },
    ".red": {
      color: "ff0000"
    },
    ".bold": {
      fontWeight: "bold"
    },
    ".big": {
      fontSize: 25
    },
    ".small": {
      fontSize: 12
    },
    unorderedListMarker: {
      color: "000000"
    },
    table:{
      scrollX: true
    },
    tableCell:{
      maxWidth: 150
    }
  }
  private servalMarkdownConfig: ServalMarkdownConfig = {
    animationType: "typewriter",
    animationVelocity: 20,
    enableSelection: true,
    sourceType: "markdown",
    selectionHandleColor: "79bbff",
    selectionHighlightColor: "6679bbff"
  }
  private content: string = `
# The following is plain text:

![](inlineview://aaa)

This is a plain text paragraph, ***111****1* containing **bold**, *italic*, ***bold + testtetstest italic***, ***italic* mixed with bold**, ~~strikethrough~~, \`inline code\`, <span class="mark">these <span class="red">red <span class="bold">bold <span class="underline">underlined</span><span class="big">large font <span class="small">*small italic*</span></span></span></span> mixed inline styles</span>, and emoji: 😄

### The following is a blockquote:

> Reading a good book is like talking to a noble soul. [Goethe](url://link)
> \`\`\`
> code block in quote code block in quote code block in quote code block in quote code block in quote
> \`\`\`
> The employment system is unfavorable to workers, but workers are powerless to break free from it. Ruan Yifeng

![](inlineview://aaa)

-----

### The following are lists:

1. Ordered list 1
    1. Nested ordered list 1
    2. Nested ordered list 2
    3. Nested ordered list 3
2. Ordered list 2
3. Ordered list 3

- Unordered list 1
  - Unordered list 2
  - Unordered list 3
- Unordered list 4

### The following is a table:

| Programming Language | Creator | Year | Primary Application |
| :------- | :----------- | :------- | :----------------- |
| Python   | Guido van Rossum | 1991    | Data analysis, AI development |
| Java     | James Gosling   | 1995    | Enterprise development, Android |
| JavaScript | Brendan Eich   | 1995    | Frontend development, Full‑stack development |
| Go       | Robert Griesemer et al. | 2009   | Cloud computing, Backend services |

`
  private contentLength = 1

  build() {
    Row() {
      ContentSlot(this.servalMarkdownView)
    }
  }

  appendContent() {
    if (this.contentLength >= this.content.length) {
      return
    }
    this.contentLength += 2000;
    this.servalMarkdownView.setContent(this.content.substring(0, this.contentLength))
    // setTimeout(() => {
    //   this.appendContent()
    // }, 100)
  }

  onDidBuild(): void {
    this.servalMarkdownView.setContent(this.content.substring(0, this.contentLength))
    this.servalMarkdownView.setStyle(this.servalMarkdownStyle)
    this.servalMarkdownView.setConfig(this.servalMarkdownConfig)
    let inlineViewLoader = (id: string) => {
      let builder = createNode(this.getUIContext())
      return builder.getFrameNode()
    }
    let replacementViewLoader = (ud: number) => {
      let builder = createNode(this.getUIContext())
      return builder.getFrameNode()
    }
    this.servalMarkdownView.registerInlineViewLoader(inlineViewLoader)
    this.servalMarkdownView.registerReplacementViewLoader(replacementViewLoader)
    this.bindEvents()
    this.bindExposure()
    this.appendContent()
  }

  bindEvents(): void {
    this.servalMarkdownView.bindEvent("parseEnd", () => {
      console.log("serval_markdown: parseEnd")
    })
    this.servalMarkdownView.bindEvent("drawStart", () => {
      console.log("serval_markdown: drawStart")
    })
    this.servalMarkdownView.bindEvent("drawEnd", () => {
      console.log("serval_markdown: drawEnd")
    })
    this.servalMarkdownView.bindEvent("animationStep", (current: number, max: number) => {
      console.log("serval_markdown: animationStep:" + current + " max:" + max)
    })
    this.servalMarkdownView.bindEvent("linkClicked", (url: string, content: string) => {
      console.log("serval_markdown: link: " + url + " content:" + content + " clicked")
    });
    this.servalMarkdownView.bindEvent("imageClicked", (url: string) => {
      console.log("serval_markdown: image: " + url + " clicked")
    })
    this.servalMarkdownView.bindEvent("selectionChanged", (start: number, end: number, direction: string, state:string) => {
      console.log("serval_markdown: selectionChanged: " + start + "," + end + " direction:" + direction + " state:" + state)
    })
  }

  bindExposure(): void {
    this.servalMarkdownView.bindExposure("imageAppear", (url: string) => {
      console.log("serval_markdown: image: " + url + " appear")
    })
    this.servalMarkdownView.bindExposure("imageDisappear", (url: string) => {
      console.log("serval_markdown: image: " + url + " disappear")
    })
    this.servalMarkdownView.bindExposure("linkAppear", (url: string, content: string) => {
      console.log("serval_markdown: link: " + url + ":" + content + " appear")
    })
    this.servalMarkdownView.bindExposure("linkDisappear", (url: string, content: string) => {
      console.log("serval_markdown: link: " + url + ":" + content + " disappear")
    })
  }
}
