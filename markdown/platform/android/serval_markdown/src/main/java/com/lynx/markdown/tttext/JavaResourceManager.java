// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown.tttext;

import android.graphics.Typeface;
import com.lynx.textra.JavaFontManager;
import com.lynx.textra.JavaTypeface;
import com.lynx.textra.TTText;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.Map;

public class JavaResourceManager {
  private final ArrayList<IRunDelegate> mRunDelegateList;
  private final Map<Object, Integer> mIDMap;
  private final JavaFontManager mFontManager;

  public JavaResourceManager() {
    mRunDelegateList = new ArrayList<>();
    mRunDelegateList.add(null);
    mIDMap = new Hashtable<>();
    mFontManager = TTText.mFontManager;
  }

  public int add(IRunDelegate run_delegate) {
    if (run_delegate == null)
      return 0;
    int id = find(run_delegate);
    if (id != -1)
      return id;
    mRunDelegateList.add(run_delegate);
    id = mRunDelegateList.size() - 1;
    mIDMap.put(run_delegate, id);
    return id;
  }

  public JavaTypeface add(Typeface font, String families) {
    if (font == null) {
      families = "";
    }
    return mFontManager.CreateOrRegisterTypeface(font, families, 400, false);
  }

  public IRunDelegate getRunDelegate(int id) {
    if (id < 0 || id >= mRunDelegateList.size()) {
      return null;
    }
    return mRunDelegateList.get(id);
  }

  public JavaTypeface getFont(int id) {
    return mFontManager.GetTypefaceByIndex(id);
  }

  private int find(Object obj) {
    if (obj == null)
      return -1;
    Integer id = mIDMap.get(obj);
    if (id == null) {
      return -1;
    }
    return id;
  }
}
