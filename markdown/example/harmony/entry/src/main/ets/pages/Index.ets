import { Markdown } from './Markdown';

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  build() {
    List() {
      Markdown()
    }.width('100%').height('100%')
  }
}