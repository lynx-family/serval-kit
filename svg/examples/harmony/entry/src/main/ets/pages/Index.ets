import { SvgData, SvgView } from 'servalsvg';
import { Size } from '@kit.ArkUI';

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';
  svgSize: Size = { width: 100, height: 100 };
  @State data1: SvgData = {
    width: 100, height: 100, content: "<svg viewBox=\"0 0 100 100\" xmlns=\"http://www.w3.org/2000/svg\">\n" +
      "  <line x1=\"0\" y1=\"80\" x2=\"100\" y2=\"20\" stroke=\"blue\" fill=\"purple\"\n" +
      "       fill-opacity=\"0.5\" stroke-opacity=\"0.8\"  />\n" +
      "\n" +
      "</svg>"
  };
  @State data2: SvgData = {
    width: 100, height: 100, content: "<svg viewBox=\"0 0 100 100\" xmlns=\"http://www.w3.org/2000/svg\">\n" +
      "  <circle cx=\"50\" cy=\"50\" r=\"50\" stroke=\"blue\" fill=\"purple\"\n" +
      "       fill-opacity=\"0.8\" stroke-opacity=\"0.8\"/>\n" +
      "</svg>"
  };
  @State data3: SvgData = {
    width: 100, height: 100, content: "<svg viewBox=\"0 0 200 100\" xmlns=\"http://www.w3.org/2000/svg\">\n" +
      "  <!-- Example of a polygon with the default fill -->\n" +
      "  <polygon points=\"0,100 50,25 50,75 100,0\" />\n" +
      "\n" +
      "  <!-- Example of the same polygon shape with stroke and no fill -->\n" +
      "  <polygon points=\"100,100 150,25 150,75 200,0\" fill=\"none\" stroke=\"black\" />\n" +
      "</svg>"
  }
  @State data4: SvgData = {
    width: 100, height: 100, content: "<svg viewBox=\"0 0 200 100\" xmlns=\"http://www.w3.org/2000/svg\">\n" +
      "  <!-- Example of a polyline with the default fill -->\n" +
      "  <polyline points=\"0,100 50,25 50,75 100,0\" />\n" +
      "\n" +
      "  <!-- Example of the same polyline shape with stroke and no fill -->\n" +
      "  <polyline points=\"100,100 150,25 150,75 200,0\" fill=\"none\" stroke=\"black\" />\n" +
      "</svg>"
  }
  @State data5: SvgData = {
    width: 100,
    height: 100,
    content: "<svg viewBox='70.5166 72.254 92.3394 10.2516'>\n" +
      "            <defs>\n" +
      "              <path d='M4.65056 -3.32354L2.25953 -5.70262C2.11606 -5.84608 2.09215 -5.86999 1.99651 -5.86999C1.87696 -5.86999 1.75741 -5.76239 1.75741 -5.63088C1.75741 -5.5472 1.78132 -5.52329 1.91283 -5.39178L4.30386 -2.98879L1.91283 -0.585803C1.78132 -0.454296 1.75741 -0.430386 1.75741 -0.3467C1.75741 -0.215193 1.87696 -0.107597 1.99651 -0.107597C2.09215 -0.107597 2.11606 -0.131507 2.25953 -0.274969L4.63861 -2.65405L7.11333 -0.179328C7.13724 -0.167372 7.22092 -0.107597 7.29265 -0.107597C7.43611 -0.107597 7.53176 -0.215193 7.53176 -0.3467C7.53176 -0.37061 7.53176 -0.418431 7.49589 -0.478207C7.48394 -0.502117 5.58306 -2.37908 4.98531 -2.98879L7.1731 -5.17659C7.23288 -5.24832 7.4122 -5.40374 7.47198 -5.47547C7.48394 -5.49938 7.53176 -5.5472 7.53176 -5.63088C7.53176 -5.76239 7.43611 -5.86999 7.29265 -5.86999C7.19701 -5.86999 7.14919 -5.82217 7.01768 -5.69066L4.65056 -3.32354Z' id='g0-2'/>\n" +
      "              <path d='M5.35592 -3.82565C5.35592 -4.81793 5.29614 -5.7863 4.86575 -6.69489C4.37559 -7.68717 3.51482 -7.95019 2.92902 -7.95019C2.23562 -7.95019 1.3868 -7.60349 0.944458 -6.61121C0.609714 -5.85803 0.490162 -5.11681 0.490162 -3.82565C0.490162 -2.666 0.573848 -1.79328 1.00423 -0.944458C1.47049 -0.0358655 2.29539 0.251059 2.91706 0.251059C3.95716 0.251059 4.55492 -0.37061 4.90162 -1.06401C5.332 -1.96065 5.35592 -3.13225 5.35592 -3.82565ZM2.91706 0.0119552C2.5345 0.0119552 1.75741 -0.203238 1.53026 -1.50635C1.39875 -2.22366 1.39875 -3.13225 1.39875 -3.96912C1.39875 -4.94944 1.39875 -5.83412 1.59004 -6.53948C1.79328 -7.34047 2.40299 -7.71108 2.91706 -7.71108C3.37136 -7.71108 4.06476 -7.43611 4.29191 -6.40797C4.44732 -5.72653 4.44732 -4.78207 4.44732 -3.96912C4.44732 -3.16812 4.44732 -2.25953 4.31582 -1.53026C4.08867 -0.215193 3.33549 0.0119552 2.91706 0.0119552Z' id='g2-48'/>\n" +
      "              <path d='M3.44309 -7.66326C3.44309 -7.93823 3.44309 -7.95019 3.20399 -7.95019C2.91706 -7.6274 2.3193 -7.18506 1.08792 -7.18506V-6.83836C1.36289 -6.83836 1.96065 -6.83836 2.61818 -7.14919V-0.920548C2.61818 -0.490162 2.58232 -0.3467 1.53026 -0.3467H1.15965V0C1.48244 -0.0239103 2.64209 -0.0239103 3.03661 -0.0239103S4.57883 -0.0239103 4.90162 0V-0.3467H4.53101C3.47895 -0.3467 3.44309 -0.490162 3.44309 -0.920548V-7.66326Z' id='g2-49'/>\n" +
      "              <path d='M8.06974 -3.87347C8.23711 -3.87347 8.4523 -3.87347 8.4523 -4.08867C8.4523 -4.31582 8.24907 -4.31582 8.06974 -4.31582H1.02814C0.860772 -4.31582 0.645579 -4.31582 0.645579 -4.10062C0.645579 -3.87347 0.848817 -3.87347 1.02814 -3.87347H8.06974ZM8.06974 -1.64981C8.23711 -1.64981 8.4523 -1.64981 8.4523 -1.86501C8.4523 -2.09215 8.24907 -2.09215 8.06974 -2.09215H1.02814C0.860772 -2.09215 0.645579 -2.09215 0.645579 -1.87696C0.645579 -1.64981 0.848817 -1.64981 1.02814 -1.64981H8.06974Z' id='g2-61'/>\n" +
      "              <path d='M2.19975 -0.573848C2.19975 -0.920548 1.91283 -1.15965 1.6259 -1.15965C1.2792 -1.15965 1.0401 -0.872727 1.0401 -0.585803C1.0401 -0.239103 1.32702 0 1.61395 0C1.96065 0 2.19975 -0.286924 2.19975 -0.573848Z' id='g1-58'/>\n" +
      "            </defs>\n" +
      "            <g id='page1' transform='matrix(1.25 0 0 1.25 0 0)'>\n" +
      "              <use x='56.4133' xlink:href='#g2-49' y='65.7534'/>\n" +
      "              <use x='62.2444' xlink:href='#g2-48' y='65.7534'/>\n" +
      "              <use x='68.0755' xlink:href='#g2-48' y='65.7534'/>\n" +
      "              <use x='76.5633' xlink:href='#g0-2' y='65.7534'/>\n" +
      "              <use x='88.4837' xlink:href='#g2-48' y='65.7534'/>\n" +
      "              <use x='94.3148' xlink:href='#g1-58' y='65.7534'/>\n" +
      "              <use x='97.5544' xlink:href='#g2-49' y='65.7534'/>\n" +
      "              <use x='106.706' xlink:href='#g2-61' y='65.7534'/>\n" +
      "              <use x='119.098' xlink:href='#g2-49' y='65.7534'/>\n" +
      "              <use x='124.929' xlink:href='#g2-48' y='65.7534'/>\n" +
      "            </g>\n" +
      "          </svg>"
  }
  @State data6: SvgData = {
    width: 100,
    height: 100,
    antiAlias: false,
    content: " <svg xmlns=\"http://www.w3.org/2000/svg\" width=\"13.638ex\" height=\"2.773ex\" role=\"img\" focusable=\"false\" viewBox=\"0 -864.9 6028 1225.5\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" aria-hidden=\"true\" style=\"vertical-align: -0.816ex;\">\n" +
      "\n" +
      "            <defs>\n" +
      "              <path color=\"red\" id=\"MJX-12-TEX-N-32\" d=\"M109 429Q82 429 66 447T50 491Q50 562 103 614T235 666Q326 666 387 610T449 465Q449 422 429 383T381 315T301 241Q265 210 201 149L142 93L218 92Q375 92 385 97Q392 99 409 186V189H449V186Q448 183 436 95T421 3V0H50V19V31Q50 38 56 46T86 81Q115 113 136 137Q145 147 170 174T204 211T233 244T261 278T284 308T305 340T320 369T333 401T340 431T343 464Q343 527 309 573T212 619Q179 619 154 602T119 569T109 550Q109 549 114 549Q132 549 151 535T170 489Q170 464 154 447T109 429Z\"></path>\n" +
      "\n" +
      "              <path id=\"MJX-12-TEX-N-39\" d=\"M352 287Q304 211 232 211Q154 211 104 270T44 396Q42 412 42 436V444Q42 537 111 606Q171 666 243 666Q245 666 249 666T257 665H261Q273 665 286 663T323 651T370 619T413 560Q456 472 456 334Q456 194 396 97Q361 41 312 10T208 -22Q147 -22 108 7T68 93T121 149Q143 149 158 135T173 96Q173 78 164 65T148 49T135 44L131 43Q131 41 138 37T164 27T206 22H212Q272 22 313 86Q352 142 352 280V287ZM244 248Q292 248 321 297T351 430Q351 508 343 542Q341 552 337 562T323 588T293 615T246 625Q208 625 181 598Q160 576 154 546T147 441Q147 358 152 329T172 282Q197 248 244 248Z\"></path>\n" +
      "\n" +
      "              <path id=\"MJX-12-TEX-N-D7\" d=\"M630 29Q630 9 609 9Q604 9 587 25T493 118L389 222L284 117Q178 13 175 11Q171 9 168 9Q160 9 154 15T147 29Q147 36 161 51T255 146L359 250L255 354Q174 435 161 449T147 471Q147 480 153 485T168 490Q173 490 175 489Q178 487 284 383L389 278L493 382Q570 459 587 475T609 491Q630 491 630 471Q630 464 620 453T522 355L418 250L522 145Q606 61 618 48T630 29Z\"></path>\n" +
      "\n" +
      "              <path id=\"MJX-12-TEX-N-31\" d=\"M213 578L200 573Q186 568 160 563T102 556H83V602H102Q149 604 189 617T245 641T273 663Q275 666 285 666Q294 666 302 660V361L303 61Q310 54 315 52T339 48T401 46H427V0H416Q395 3 257 3Q121 3 100 0H88V46H114Q136 46 152 46T177 47T193 50T201 52T207 57T213 61V578Z\"></path>\n" +
      "\n" +
      "              <path id=\"MJX-12-TEX-N-38\" d=\"M70 417T70 494T124 618T248 666Q319 666 374 624T429 515Q429 485 418 459T392 417T361 389T335 371T324 363L338 354Q352 344 366 334T382 323Q457 264 457 174Q457 95 399 37T249 -22Q159 -22 101 29T43 155Q43 263 172 335L154 348Q133 361 127 368Q70 417 70 494ZM286 386L292 390Q298 394 301 396T311 403T323 413T334 425T345 438T355 454T364 471T369 491T371 513Q371 556 342 586T275 624Q268 625 242 625Q201 625 165 599T128 534Q128 511 141 492T167 463T217 431Q224 426 228 424L286 386ZM250 21Q308 21 350 55T392 137Q392 154 387 169T375 194T353 216T330 234T301 253T274 270Q260 279 244 289T218 306L210 311Q204 311 181 294T133 239T107 157Q107 98 150 60T250 21Z\"></path>\n" +
      "\n" +
      "              <path id=\"MJX-12-TEX-N-30\" d=\"M96 585Q152 666 249 666Q297 666 345 640T423 548Q460 465 460 320Q460 165 417 83Q397 41 362 16T301 -15T250 -22Q224 -22 198 -16T137 16T82 83Q39 165 39 320Q39 494 96 585ZM321 597Q291 629 250 629Q208 629 178 597Q153 571 145 525T137 333Q137 175 145 125T181 46Q209 16 250 16Q290 16 318 46Q347 76 354 130T362 333Q362 478 354 524T321 597Z\"></path>\n" +
      "\n" +
      "              <path id=\"MJX-12-TEX-N-3D\" d=\"M56 347Q56 360 70 367H707Q722 359 722 347Q722 336 708 328L390 327H72Q56 332 56 347ZM56 153Q56 168 72 173H708Q722 163 722 153Q722 140 707 133H70Q56 140 56 153Z\"></path>\n" +
      "\n" +
      "              <path id=\"MJX-12-TEX-N-2218\" d=\"M55 251Q55 328 112 386T249 444T386 388T444 249Q444 171 388 113T250 55Q170 55 113 112T55 251ZM245 403Q188 403 142 361T96 250Q96 183 141 140T250 96Q284 96 313 109T354 135T375 160Q403 197 403 250Q403 313 360 358T245 403Z\"></path>\n" +
      "            </defs>\n" +
      "\n" +
      "              <g  stroke-width=\"0\" transform=\"scale(1,-1)\">\n" +
      "                <g data-mml-node=\"math\">\n" +
      "                    <g data-mml-node=\"mfrac\">\n" +
      "                      <g data-mml-node=\"mrow\" transform=\"translate(220,394) scale(0.707)\">\n" +
      "                        <g data-mml-node=\"mn\">\n" +
      "                          <use data-c=\"32\" xlink:href=\"#MJX-12-TEX-N-32\"></use>\n" +
      "                          <use data-c=\"39\" xlink:href=\"#MJX-12-TEX-N-39\" transform=\"translate(500,0)\"></use>\n" +
      "                        </g>\n" +
      "                        <g data-mml-node=\"mo\" transform=\"translate(1000,0)\">\n" +
      "                          <use data-c=\"D7\" xlink:href=\"#MJX-12-TEX-N-D7\"></use>\n" +
      "                        </g>\n" +
      "                        <g data-mml-node=\"mn\" transform=\"translate(1778,0)\">\n" +
      "                          <use data-c=\"31\" xlink:href=\"#MJX-12-TEX-N-31\"></use>\n" +
      "                          <use data-c=\"38\" xlink:href=\"#MJX-12-TEX-N-38\" transform=\"translate(500,0)\"></use>\n" +
      "                          <use data-c=\"30\" xlink:href=\"#MJX-12-TEX-N-30\" transform=\"translate(1000,0)\"></use>\n" +
      "                        </g>\n" +
      "                      </g>\n" +
      "                      <g data-mml-node=\"mn\" transform=\"translate(1025.4,-345) scale(0.707)\">\n" +
      "                        <use data-c=\"31\" xlink:href=\"#MJX-12-TEX-N-31\"></use>\n" +
      "                        <use data-c=\"38\" xlink:href=\"#MJX-12-TEX-N-38\" transform=\"translate(500,0)\"></use>\n" +
      "                      </g>\n" +
      "                      <rect width=\"2517.9\" height=\"60\" x=\"120\" y=\"220\"></rect>\n" +
      "                    </g>\n" +
      "\n" +
      "                    <g data-mml-node=\"mo\" transform=\"translate(3035.7,0)\">\n" +
      "                      <use data-c=\"3D\" xlink:href=\"#MJX-12-TEX-N-3D\"></use>\n" +
      "                    </g>\n" +
      "\n" +
      "                    <g data-mml-node=\"msup\" transform=\"translate(4091.5,0)\">\n" +
      "                      <g data-mml-node=\"mn\">\n" +
      "                        <use data-c=\"32\" xlink:href=\"#MJX-12-TEX-N-32\"></use>\n" +
      "                        <use data-c=\"39\" xlink:href=\"#MJX-12-TEX-N-39\" transform=\"translate(500,0)\"></use>\n" +
      "                        <use data-c=\"30\" xlink:href=\"#MJX-12-TEX-N-30\" transform=\"translate(1000,0)\"></use>\n" +
      "                      </g>\n" +
      "                      <g data-mml-node=\"mo\" transform=\"translate(1533,393.1) scale(0.707)\">\n" +
      "                        <use data-c=\"2218\" xlink:href=\"#MJX-12-TEX-N-2218\"></use>\n" +
      "                      </g>\n" +
      "                    </g>\n" +
      "\n" +
      "                  </g>\n" +
      "                </g>\n" +
      "              </svg>"
  }
  @State data7: SvgData = {
    width: 100, height: 100, content: "<svg\n" +
      "  xmlns=\"http://www.w3.org/2000/svg\"\n" +
      "  xmlns:xlink=\"http://www.w3.org/1999/xlink\"" +
      "  viewBox=\"0 0 10 10\"\n" +
      ">\n" +
      "  <defs>\n" +
      "    <linearGradient id=\"grad1\"  x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\">\n" +
      "      <stop offset=\"0.05\" stop-color=\"gold\" />\n" +
      "      <stop offset=\"0.95\" stop-color=\"red\" />\n" +
      "    </linearGradient>\n" +
      "  </defs>\n" +
      "  <circle cx=\"5\" cy=\"5\" r=\"4\" fill=\"url('#grad1')\" />\n" +
      "</svg>\n"
  }
  @State data8: SvgData = {
    width: 200,
    height: 200,
    content: "<svg viewBox=\"0 0 48 48\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"M 9.82 24.53l 0.5 -5.22a 0.5 0.5 0 0 1 0.86 -0.3l 2.58 2.57a 1 1 0 0 1 0.29 0.8l -0.25 2.54c -0.28 2.85 -1.86 4.92 -3.56 7.08h 13.52a 1 1 0 0 1 0.7 0.3l 2.86 2.85a 0.5 0.5 0 0 1 -0.35 0.85H 6.12a 2 2 0 0 1 -1.57 -3.24c 1.87 -2.37 4.96 -5.04 5.27 -8.23ZM 4.46 6.63L 41.7 43.87a 1 1 0 0 0 1 -0.25l 1.42 -1.41a 1 1 0 0 0 0 -1.42L 39.33 36h 2.55a 2 2 0 0 0 1.57 -3.24c -1.87 -2.37 -4.96 -5.04 -5.27 -8.23l -1.02 -10.59A 13.22 13.22 0 0 0 12.33 9l -4.8 -4.8a 1 1 0 0 0 -1.4 0L 4.7 5.63a 1 1 0 0 0 -0.25 1.01Zm 10.9 5.4a 9.22 9.22 0 0 1 17.81 2.3l 1.03 10.59c 0.28 2.85 1.86 4.92 3.56 7.08h -2.43L 15.35 12.02ZM 30.9 40.98c 0.11 -0.54 -0.35 -0.99 -0.9 -0.99H 18c -0.55 0 -1.01 0.45 -0.9 1c 1.28 6.61 12.52 6.61 13.8 0Z\" fill=\"rgba(0, 0, 0, 0.56)\"/></svg>",
  }

  constructor()

  build() {
    RelativeContainer() {
      // SvgView({ svgData: this.data1 }).backgroundColor(Color.Red).position({ x: 100, y: 0 }).onClick(() => {
      //   this.data1.width = this.data1.width + 10;
      // });
      // SvgView({ svgData: this.data8 }).position({ x: 100, y: 100 });
      // SvgView({ svgData: this.data1 }).position({ x: 0, y: 0 });
      // SvgView({ svgData: this.data2 }).backgroundColor(Color.Yellow).position({ x: 100, y: 100 });
      // SvgView({ svgData: this.data3 }).backgroundColor(Color.Yellow).position({ x: 100, y: 200 });
      // SvgView({ svgData: this.data4 }).backgroundColor(Color.Yellow).position({ x: 100, y: 300 });
      // SvgView({ svgData: this.data5 }).backgroundColor(Color.Yellow).position({ x: 100, y: 400 });
      SvgView({ svgData: this.data6 }).backgroundColor(Color.Yellow).position({ x: 100, y: 500 })
      SvgView({ svgData: this.data7 }).backgroundColor(Color.Yellow).position({ x: 100, y: 600 })
      // Text(this.message)
      //   .id('HelloWorld')
      //   .fontSize(50)
      //   .fontWeight(FontWeight.Bold)
      //   .alignRules({
      //     center: { anchor: '__container__', align: VerticalAlign.Center },
      //     middle: { anchor: '__container__', align: HorizontalAlign.Center }
      //   })
    }
    .height('100%')
    .width('100%')
  }
}