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
  private final ArrayList<IRunDelegate> run_delegate_list_;
  private final Map<Object, Integer> id_map_;
  private JavaFontManager font_manager_;

  public JavaResourceManager() {
    run_delegate_list_ = new ArrayList<>();
    run_delegate_list_.add(null);
    id_map_ = new Hashtable<>();
    font_manager_ = TTText.mFontManager;
  }

  public int add(IRunDelegate run_delegate) {
    if (run_delegate == null)
      return 0;
    int id = find(run_delegate);
    if (id != -1)
      return id;
    run_delegate_list_.add(run_delegate);
    id = run_delegate_list_.size() - 1;
    id_map_.put(run_delegate, id);
    return id;
  }

  public JavaTypeface add(Typeface font, String families) {
    if (font == null) {
      families = "";
    }
    return font_manager_.CreateOrRegisterTypeface(font, families, 400, false);
  }

  public IRunDelegate getRunDelegate(int id) {
    if (id < 0 || id >= run_delegate_list_.size()) {
      return null;
    }
    return run_delegate_list_.get(id);
  }

  public JavaTypeface getFont(int id) {
    return font_manager_.GetTypefaceByIndex(id);
  }

  private int find(Object obj) {
    if (obj == null)
      return -1;
    Integer id = id_map_.get(obj);
    if (id == null) {
      return -1;
    }
    return id;
  }
}
