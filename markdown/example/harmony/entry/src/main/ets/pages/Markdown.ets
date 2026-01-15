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
    }
  }
  private servalMarkdownConfig: ServalMarkdownConfig = {
    animationType: "none",
    animationVelocity: 20,
    enableSelection: true,
    sourceType: "markdown",
    selectionHandleColor: "79bbff",
    selectionHighlightColor: "6679bbff"
  }
  private content: string = `
# 以下是普通文本：

![](inlineview://aaa)

这是一段普通文本，***111****1*包含**粗体**、*斜体*、***粗体+ testtetstest 斜体***、***斜体*粗体混合**、~~删除线~~、\`inline code\`，<span class="mark">这些<span class="red">红色<span class="bold">加粗<span class="underline">下划线</span><span class="big">大字号<span class="small">*小斜体*</span></span></span></span>混合的行内的样式</span>，以及表情：😄

### 以下是引用块：

> 读一本好书，就是在和高尚的人谈话。 [歌德](url://link)
> \`\`\`
> code block in quote code block in quote code block in quote code block in quote code block in quote
> \`\`\`
> 雇用制度对工人不利，但工人根本无力摆脱这个制度。阮一峰

![](inlineview://aaa)

-----

### 以下是列表：

1. 有序列表1
    1. 嵌套有序列表1
    2. 嵌套有序列表2
    3. 嵌套有序列表3
2. 有序列表2
3. 有序列表3

- 无序列表1
  - 无序列表2
  - 无序列表3
- 无序列表4

### 以下是表格：

| 编程语言 | 创始人       | 发布年份 | 主要应用领域       |
| :------- | :----------- | :------- | :----------------- |
| Python   | Guido van Rossum | 1991    | 数据分析、AI开发   |
| Java     | James Gosling   | 1995    | 企业级开发、Android |
| JavaScript | Brendan Eich   | 1995    | 前端开发、全栈开发 |
| Go       | Robert Griesemer 等 | 2009   | 云计算、后端服务   |

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
