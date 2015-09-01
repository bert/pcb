#!/bin/bash

##
# \file antifork/antifork.sh
#
# \brief Main script for the Antifork system.
#
# <hr>
#
# <h1><b>Copyright.</b></h1>\n
#
# Antifork for
# PCB, interactive printed circuit board design
# Copyright (C) 2015      Markus "Traumflug" Hitter <mah@jump-ing.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#
# Antifork fetches from known, forked repositores and creates local branches
# of their branches. The point is to make all the hidden stuff visible, to
# allow easy cherry-picking, to allow rebasing, and also to deal with forks
# using StGit (which hilariously messes up a Git repository in a way not
# viewable in gitk).
#
# Along the way, branches in forks can be filtered. This is useful for
# forked branches which simply duplicate branches in the official repo,
# which handle topics already finished and similar stuff.
#
# Antifork is meant to run in local clones, not in the official repo. Copied
# branches are prefixed with the name of the fork when fetched for the first
# time. If a forked branch got updated in the fork and is fetched again, it
# additionally gets the prefix "today-". This means to make it easy to
# recognize changes in hte fork.
#
# Happy antiforking, everybody!
#
#
# Format of the repository description files:
#
#  - The name of the file is also the prefix of the local branch copies.
#
#  - First line is the repository URL.
#
#  - Lines starting with 'ignorebranch' followed by the hash of the last
#    commit of a branch mark remote branches which should be ignored.
#    Typically this is the case when all commits in a branch were reviewed and
#    found to be not helpful or the branch was picked to the official repo
#    already.
#
#    It's the last known (stale) commit instead of the branch name,
#    because if there appear new commits on the branch, the branch
#    should be reviewed again.
#
#
# TODO: also allow to ignore single commits, for example by changing their
#       commit message to "obsolete" and squashing consecutive obsolete commits.
#       This should help reviewing a lot.
#
#       Not exactly trivial, because this requires rebasing and rebasing
#       changes the commit hash.

BASE_CMD=$(basename "$0")

if ! ls -d "${BASE_CMD}" >/dev/null 2>&1; then
  echo "Run ${BASE_CMD} from the directory it sits in."
  exit 1
fi

# Handle parameters.
while [ "${1}" != "" ]; do
  case "${1}" in
    -h|--help)
      echo
      echo "  This script attaches a number of known repositores and creates"
      echo "  local branches from their branches. The point is to make all the"
      echo "  hidden stuff visible."
      echo
      echo "  -h,--help    Show this help."
      echo
      echo "  --also-svn   Also attach and fetch forks done in Subversion"
      echo "               repositories. These integrate not too well, so"
      echo "               they're ignored by default."
      echo
      echo "  Running ${BASE_CMD} without parameters fetches these"
      echo "  repositories:"
      echo
      echo "    name              location"
      echo
      for REPO in *; do
        if [ "${REPO}" != "${BASE_CMD}" ]; then
          printf "    %-16s  " "${REPO}"
          head -1 "${REPO}"
        fi
      done
      echo
      echo "  Passing one or more of the names as parameters fetches only the"
      echo "  given ones."
      exit
      ;;
    --also-svn)
      ALSO_SVN="true"
      shift
      ;;
    *)
      if [ -r "${1}" ]; then
        REPOS=("${REPOS[@]}" "${1}")
      else
        echo -n "$(basename "$0"): no description file found for repository "
        echo    "'${1}'. Dropping."
      fi
      shift
      ;;
  esac
done

# No parameters case.
if [ ${#REPOS[*]} = 0 ]; then
  for REPO in *; do
    if [ "${REPO}" != "${BASE_CMD}" ]; then
      REPOS=("${REPOS[@]}" "${REPO}")
    fi
  done
fi


# Get into action on Git forks.
for REPO in "${REPOS[@]}"; do

  URL=$(head -1 "${REPO}")
  if [ "${URL}" != "${URL#svn}" ]; then
    echo "Eeeek! ${REPO} = ${URL} is a Subversion repo!"
    continue
  fi

  git remote add "${REPO}" "${URL}"
  echo -n "Fetching ${REPO} ..."
  git fetch "${REPO}" >/dev/null
  echo " done."

  git branch -r | grep "${REPO}/" | grep -v ".stgit" | \
    while read BRANCH; do
      LOCAL_BRANCH=$(echo "${BRANCH}" | tr -d ' ' | tr / -)
      TODAY_BRANCH="today-${LOCAL_BRANCH}"
      RAW_BRANCH=$(git show -q "${BRANCH}" | awk '{ print $2; exit; }')

      if cat "${REPO}" | grep "^ignorebranch" | grep -q "${RAW_BRANCH}"; then
        echo "Ignoring ${BRANCH}."
        if git show -q "${LOCAL_BRANCH}" >/dev/null 2>&1; then
          git branch -D "${LOCAL_BRANCH}"
        fi
        continue
      else
        echo "Handling ${BRANCH}."
      fi

      if git show -q "${LOCAL_BRANCH}" >/dev/null 2>&1; then
        # Local branch exists.
        if git show -q "${TODAY_BRANCH}" >/dev/null 2>&1; then
          # Today branch exists. Make it the new LOCAL_BRANCH.
          git branch -M "${TODAY_BRANCH}" "${LOCAL_BRANCH}"
        fi

        # If the remote branch changed, make it a TODAY_BRANCH.
        if [ "$(git show -q "${BRANCH}" | head -1)" != \
             "$(git show -q "${LOCAL_BRANCH}" | head -1)" ]
        then
          git branch "${TODAY_BRANCH}" "${BRANCH}"
        fi
      else
        # NO local branch.
        git branch "${LOCAL_BRANCH}" "${BRANCH}"
      fi

    done

  git remote remove "${REPO}"

done

if [ -z "${ALSO_SVN}" ]; then
  echo "Ignoring forks in Subversion repositories."
  exit 0
fi

# Handle Subversion repos.
#
# Cloning a Subversion repo is a very time consuming operation, so we handle
# these entirely different:
#
#  - Don't drop the clone after being done. Actually, Git has no function to
#    remove a SVN remote repo once added. See below on how to do it anyways.
#
#  - Just fetch the branches, no further handling.
#
# Advantage for our purposes here is, branching an dealing with branches
# is rather expensive with SVN, so developers using SVN do so rarely. They
# also can't rebase, so even topic branches can't change.
for REPO in "${REPOS[@]}"; do

  URL=$(head -1 "${REPO}")
  if [ "${URL}" = "${URL#svn}" ]; then
    continue
  fi

  ORG_PWD="$(pwd)"
  trap "cd '${ORG_PWD}'" 0

  while ! ls -d .git >/dev/null 2>&1; do
    cd ..
    if [ "$(pwd)" = "/" ]; then
      echo "Not inside a Git repo ?!?."
      exit 1
    fi
  done

  if ! git branch -a | grep -q "${REPO}/"; then
    # We have no clone, yet.
    git svn init --stdlayout --svn-remote="${REPO}" --prefix="${REPO}/" "${URL}"
  fi

  echo -n "Fetching ${REPO} ... "
  git svn fetch "${REPO}"
  echo " done."

done

#
# How to remove a SVN remote with the name XYZ from a Git repository.
#
# Note: to the best of my research there is no official Git documentation on
#       how to do this, so make sure you have a backup of your Git repo
#       (a simple copy of the directory is sufficient) and: use this on your
#       own risk!  --Traumflug 2015-08-22
#
# 1. git checkout master
#
# 2. vi .git/config
#
#    Remove the section '[svn-remote "XYZ"]'. All lines up to the next '[' or
#    to the end of file.
#
# 3. vi .git/svn/.metadata
#
#    Do the very same as in 2.
#
# 4. rm -r .git/logs/refs/remotes/XYZ
#
# 5. rm -r .git/svn/refs/remotes/XYZ
#
# 6. rm -r .git/refs/remotes/XYZ
#
# 7. git svn gc
#    git gc
#
# The remote is removed now. To also remove the branches once created by the
# SVN remote, do this:
#
# 8. git branch -a | grep remotes/rnd/ | while read B; do
#      git branch -rD "${B#remotes/}";
#    done
#
