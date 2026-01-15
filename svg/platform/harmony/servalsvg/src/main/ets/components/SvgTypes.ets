@Observed
export class SvgData {
  width: number = 0;
  height: number = 0;
  x?: number;
  y?: number;
  density?: number;
  content: string = "";
  antiAlias?: boolean;

  constructor() {
  }
}
