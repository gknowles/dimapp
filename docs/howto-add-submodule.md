<!--
Copyright Glen Knowles 2025.
Distributed under the Boost Software License, Version 1.0.
-->

# Add to another project as vendor submodule
1. Verify that branch exists on origin repo.
2. Add submodule
    - cd vendor
    - git submodule add -b <branch> https://github.com/gknowles/dimapp.git

# How to create branch for github docs
1. Create orphan gh-pages branch
    - cd vendor
    - git switch --orphan gh-pages
2. Add initial commit so the branch can be sync'd (creates branch on origin).
    - git commit --allow-empty -m "Initial gh-pages commit"
    - git push -u origin gh-pages
3. Add as vendor submodule
    - git submodule add -b gh-pages https://github.com/gknowles/dimapp.git
      gh-pages

# Outstanding Issues
