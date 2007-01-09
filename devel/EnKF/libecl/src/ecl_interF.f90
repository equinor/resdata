module ecl_inter

contains

subroutine ecl_inter_load_summary(header_file , data_file)
Implicit None
Character(Len = *), Intent(IN) :: header_file , data_file

  call ecl_inter_load_summary_(header_file , len_trim(header_file) , data_file , len_trim(data_file))

End subroutine ecl_inter_load_summary



subroutine ecl_inter_load_file(filename)
implicit none
Character(Len = *), Intent(IN) :: filename

    write(*,*) "Fortran - skal loade: ",filename
    call ecl_inter_load_file_(filename, len_trim(filename))

end subroutine ecl_inter_load_file


integer function ecl_inter_iget_kw_int(kw , iw , istep)
  Character(Len = *), Intent(IN) :: kw
  Integer, Intent(IN), Optional  :: istep   
  Integer, Intent(IN)            :: iw
  
  Integer :: Value

  if (present(istep)) then
     call ecl_inter_kw_iget_(kw , istep , iw , value)
  else
     call ecl_inter_kw_iget_(kw , 1 , iw , value)
  end if
  ecl_inter_iget_kw_int = value

end function ecl_inter_iget_kw_int



real function ecl_inter_iget_kw_real(kw , iw , istep)
Implicit None  
  Character(Len = *), Intent(IN) :: kw
  Integer, Intent(IN), Optional  :: istep
  Integer, Intent(IN)            :: iw
  
  real :: Value

  if (present(istep)) then
     call ecl_inter_kw_iget_(kw , istep , iw , value)
  else
     call ecl_inter_kw_iget_(kw , 1 , iw , value)
  end if
  ecl_inter_iget_kw_real = value

end function ecl_inter_iget_kw_real



subroutine ecl_inter_iget_kw_char(kw , iw , data , istep)
Implicit None  
  Character(Len = *), Intent(IN)  :: kw
  Integer, Intent(IN), Optional   :: istep
  Integer, Intent(IN)             :: iw
  Character(Len = *), Intent(Out) :: data
  
  If (present(istep)) then
     call ecl_inter_kw_iget_(kw , istep , iw , data)
  else
     call ecl_inter_kw_iget_(kw , 1 , iw , data)
  end If

end subroutine ecl_inter_iget_kw_char



subroutine ecl_inter_get_kw_data_real(kw , data , istep)
Implicit None  
  Character(Len = *), Intent(IN) :: kw
  Integer, Intent(IN), Optional  :: istep 
  real                           :: data(:)

  if (present(istep)) then
     call ecl_inter_kw_get_data_(kw , istep , data)
  else
     call ecl_inter_kw_get_data_(kw , 1, data)
  end if

end subroutine ecl_inter_get_kw_data_real


subroutine ecl_inter_get_kw_data_char(kw , data , istep)
Implicit None  
  Character(Len = *), Intent(IN) :: kw
  Integer, Intent(IN), Optional  :: istep 
  Character(Len=8)               :: data(:)

  if (present(istep)) then
     call ecl_inter_kw_get_data_(kw , istep , data)
  else
     call ecl_inter_kw_get_data_(kw , 1, data)
  end if

end subroutine ecl_inter_get_kw_data_char



subroutine ecl_inter_get_kw_data_int(kw , data , istep)
Implicit None  
  Character(Len = *), Intent(IN) :: kw
  Integer, Intent(IN), Optional  :: istep 
  integer                        :: data(:)

  if (present(istep)) then
     call ecl_inter_kw_get_data_(kw , istep , data)
  else
     call ecl_inter_kw_get_data_(kw , 1, data)
  end if

end subroutine ecl_inter_get_kw_data_int



integer function ecl_inter_get_kw_size(kw , istep)
Implicit None
Character(Len = *), Intent(IN) :: kw
Integer, Intent(IN), Optional  :: istep
Integer                        :: size

   If (present(istep)) then
      call ecl_inter_get_kw_size_(kw,istep,size)
   else
      call ecl_inter_get_kw_size_(kw,1,size)
   end If
   ecl_inter_get_kw_size = size

end function ecl_inter_get_kw_size



logical function ecl_inter_kw_exists(kw , istep)
Implicit None
Character(Len = *), Intent(IN) :: kw
Integer, Intent(IN), Optional  :: istep
Integer                        :: int_ex

   If (present(istep)) then
      call ecl_inter_kw_exists_(kw , istep , int_ex)
   else
      call ecl_inter_kw_exists_(kw , 1 , int_ex)
   end If

   if (int_ex == 0) then
      ecl_inter_kw_exists = .FALSE.
   else
      ecl_inter_kw_exists = .TRUE.
   end if

end function ecl_inter_kw_exists



integer function ecl_inter_get_blocks()
Implicit None
integer blocks
  
    call ecl_inter_get_blocks_(blocks) 
    ecl_inter_get_blocks = blocks

end function ecl_inter_get_blocks


subroutine ecl_inter_free
Implicit None

   call ecl_inter_free_

end subroutine ecl_inter_free
   




end module ecl_inter


