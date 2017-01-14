#!/bin/sh
#

indir=licenses
outbase=result
outdir="${outbase}/share/licenses"
docdir="${outbase}/doc"
readme="${docdir}/Readme.txt"
lic_nsh=license_include.nsh

AWK=${AWK:-awk}
MAKENSIS=${MAKENSIS:-makensis}

usage() {
cat << EOF

$0 -- build windows installer

$0 [--makensis <makensis program>] pkgname1 [pkgname2 pkgname3 ...]

Options:

  --help              : display this message and exit

  --makensis <name>   :  set the name of the makensis program

Environment Variables:

AWK        : sets the name of a awk program
MAKE       : sets the name of a GNU make program
MAKENSIS   : sets the default makensis program name

EOF
}


while test $# -gt 0 ; do
  case $1 in
    --makensis)
      MAKENSIS="$2"
      shift 2
      ;;

    --help)
      usage
      exit 0
      ;;

    -*)
      echo "$0:  ERROR: $1 is not a known option"
      exit 1
      ;;

    *)
      break
      ;;
  esac
done

pkgs="$*"
echo "pkgs = $pkgs"

lic=license.tmp.1
lic2=license.tmp.2
echo "" > "${lic}"
for pkg in $pkgs; do
  echo "Creating license table entry for $pkg"
  ./mpk license $pkg >> "${lic}" || fail
done
sort "${lic}" > "${lic2}"

rm "${lic}"

echo "" > "${lic_nsh}"

test -d "${outdir}" || mkdir -p "${outdir}"
for l in `${AWK} '{print $1}' "${lic2}" | sort -u` ; do
  lout="${outdir}/LICENSE-${l}"
  cat << EOF > "${lout}"
  The following libraries and/or programs are covered by the $l license:

EOF
  ${AWK} '$1==lic {print $2}' lic=$l "${lic2}" >> "${lout}"
  cat << EOF >> "${lout}"

-------------------------------------------------------------------

EOF
  lin="${indir}/LICENSE-${l}"
  if test -f "${lin}" ; then 
    cat "${lin}" >> ${lout}
  else
    echo "Missing license file ${lin}"
    fail
  fi
  cat << EOF >> ${lic_nsh}
!insertmacro MUI_PAGE_LICENSE "${outdir}/LICENSE-${l}"
EOF

done
rm "${lic2}"

pcb_version=`${AWK} '/AC_INIT/ {gsub(/.*,[ \t]*\[/, ""); gsub(/\]\).*/, ""); print}' ../configure.ac`
echo "pcb_version extracted from configure.ac = ${pcb_version}"
sed -e "s;@prog_version@;${pcb_version};g" pcb.nsi.in > pcb.nsi
inst_name="pcbinst-${pcb_version}.exe"

# git doesn't seem to appreciate a CRLF terminated file so build the
# DOS version on the fly
test -d "${docdir}" || mkdir -p "${docdir}"
${AWK} '{printf("%s\r\n", $0)}' Readme.txt.in > "${readme}"

echo "Generating installer..."

${MAKENSIS} pcb.nsi
rc=$?

if test $rc -eq 0 -a -f "${inst_name}" ; then
  echo "SUCCESS:  Installer created"
  ls -l "${inst_name}"
else
  echo "FAILED to create installer"
fi

exit $rc
